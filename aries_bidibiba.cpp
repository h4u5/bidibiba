/* 
 * Taylor Groves 
 * tgroves@lbl.gov
 */

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <numeric>
#include <mpi.h>
#include <map>
#include <cstdint>
#include <cstring>
#include <ctime>

static const unsigned long MSG_SIZE = 1<<20;
static const unsigned long WINDOW_SIZE = 64; 
//Assumes each rank is getting around 400MB/s
static const unsigned long MAX_LOOPS = 100000; 
//Deadline to run (in seconds)
static const unsigned long DEADLINE = 300; 
//static const unsigned long MAX_LOOPS = 100; 

using namespace std;

//Function returns a Aries Group
//Assumes that hostnames (nid####) map to groups (every 384 hosts)
int getAriesGrp(){
    char hostname[256];
    int nid;
    gethostname(hostname, 256);
    if (hostname[0] == 'n' && hostname[2] == 'd'){
        int nid = atoi((hostname+3));
        // 384 nodes in an Aries group
        // For debug purposes
        //return nid;
        return nid/384;
    }
    else{
        cerr << "Error: hostname not nid####: " << hostname << "\n";
        return -1;
    }
}

bool isZero (int i) 
{
    return i == 0;
}

class AriesGroup {
        vector <int> _members;
        const int groupID;
    public:
        AriesGroup() :
            groupID(-1)
            {}
        AriesGroup(int groupID) :
            groupID(groupID)
            {}
        AriesGroup(int groupID, vector<int> members) :
            groupID(groupID),
            _members(move(members))
            {}
        // insert a member into the group
        void addMember(int rank) {_members.push_back(rank);}
        //return the number of members
        int getSize() const {return _members.size();}
        int getID() const {return groupID;}
        void shuffle() {random_shuffle(_members.begin(), _members.end());}
        vector <int> getRanks(int k) const {
            if (k > _members.size()){
                cerr << "Warning: requesting more members (" << k << ") than in group.\n";
                return vector<int>(_members.begin(), _members.end());
            }
            else{
                return vector<int>(_members.begin(), _members.begin()+(k));
            }
        }
};

int main(int argc, char* argv[]){

    bool debug = true;
    MPI_Comm comm;
    int num_procs = 0;
    //Number of different bisection cuts we will evaluate bandwidth of
    int num_cnfg = 1;
    int my_rank, my_aries_grp = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    //Stores the Aries groups that each rank belong to
    int32_t rbuf[num_procs];
    //Sends the Processor pairs out for each index (rank)
    int32_t sbuf[num_procs];
    //Holds the nid of each rank
    int32_t nbuf[num_procs];
    //Stores the pair for this rank
    int32_t partner;
    double bw = 0;
    double tbuf[num_procs];
    char *sdata, *rdata; 
    //If we hit the deadline close out early
    int32_t stoptest;
    double t = 0.0, tstart = 0.0, tend = 0.0, tendr0 = 0.0;
    unsigned long align = sysconf(_SC_PAGESIZE);
    MPI_Request endrequest;
    MPI_Request srequest[WINDOW_SIZE];
    MPI_Request rrequest[WINDOW_SIZE];
    MPI_Status sstat[WINDOW_SIZE];
    MPI_Status rstat[WINDOW_SIZE];

    //Send the hostname/NID of each rank back to root
    char hostname[64];
    gethostname(hostname, 64);
    int32_t nid = atoi((hostname+3));
    MPI_Gather(&nid, 1, MPI_INT, &nbuf, 1, MPI_INT, 0, MPI_COMM_WORLD);

    my_aries_grp = getAriesGrp();
    if( my_aries_grp < 0 ){
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    //Lookup the ranks in a group
    map <int, AriesGroup> groups;
    MPI_Gather(&my_aries_grp, 1, MPI_INT, &rbuf, 1, MPI_INT, 0, MPI_COMM_WORLD);  
    //For number of iterations (To test different routes and intra-group contention)
    for( int i = 0; i < num_cnfg; i++){
        stoptest = 0;
        if( my_rank == 0){
            //Iterate over all the ranks and create AriesGroup structures to put in the map
            //Just changed this to start at 1 to avoid using rank 0
            for(int j = 1; j < num_procs; j++){
                //Check if the group is in the map
                if (groups.find(rbuf[j]) == groups.end()){
                    //Add a new group into the map
                    groups.insert(pair<int, AriesGroup>(rbuf[j], AriesGroup(rbuf[j])));
                    groups[rbuf[j]].addMember(j);
                }
                //Group exists in map, just add the MPI rankid to the group
                else{
                    groups[rbuf[j]].addMember(j);
                }
            }
            map <int, AriesGroup>::const_iterator mit;
            
            if(debug){
                for(mit = groups.begin(); mit != groups.end(); mit++){
                    int sizetmp = mit->second.getSize();
                    vector <int> rankstmp = mit->second.getRanks(sizetmp);
                    cout << "Group: " << mit->first << "\n";
                    cout << "\t Ranks: "; 
                    vector <int>::iterator vit;
                    for (vit = rankstmp.begin(); vit != rankstmp.end(); vit++){
                          cout << *vit << " ";
                    }
                    cout << "\n";
                }
            }
            
            //Split the groups into two sets with random pairing
            
            //Not guaranteed to get groups of equal size so need to sort groups before split
            //Create a vector of group_size, group_id for sorting.
            vector <pair<int, int>> grp_sz_id;
            
            for(mit = groups.begin(); mit != groups.end(); mit++){
                grp_sz_id.push_back(pair <int, int> (mit->second.getSize(), mit->second.getID()));
            }
            //Using reverse iterators to sort in descending
            sort(grp_sz_id.rbegin(), grp_sz_id.rend());
            
            //Determine the smallest number of nodes (assuming at least two Aries groups)
            vector <pair<int, int>>::iterator vpit;
            vpit = grp_sz_id.begin();
            if(groups.size() <= 1){
                cerr << "Failure: Must have at least two Aries Groups in allocation.  Get more nodes...\n";
                MPI_Finalize();
                exit(EXIT_FAILURE);
            }
            for (int k = 0; k < num_procs; k++){
                sbuf[k] = -1;
            }
            //Know we have at least two groups so we do this loop once at minimum
            do{
                //shuffle each AriesGroup member list to randomize communication partner
                //Does NOT shuffle across groups (Group A talks only to Group B)
                //This is more of a worst case bisection bandwidth since it forces adaptive routing
                int grp1 = vpit->second;
                vpit++;
                int grp2 = vpit->second;
                groups[grp1].shuffle();
                groups[grp2].shuffle();
                //Groups are sorted so we know grp1.size <= grp2.size
                int min_sz = groups[grp2].getSize();
                //Get two vectors of ranks that we can assign to each other
                vector <int> ranks1 = groups[grp1].getRanks(min_sz);
                vector <int> ranks2 = groups[grp2].getRanks(min_sz);
                //Create pairs randomly
                //Setup scatter buffer
                //ranks1[k] talks to ranks2[k] (Bidirectional)
                for (int k = 0; k < min_sz; k++){
                    sbuf[ranks1[k]] = ranks2[k];
                    sbuf[ranks2[k]] = ranks1[k];
                }
                //This check is only for the case where there are two groups
                if(vpit != grp_sz_id.end()){
                    vpit++;
                }
            } while (vpit < grp_sz_id.end()-1);
            if (my_rank == 0)

            MPI_Scatter(sbuf, 1, MPI_INT, &partner, 1, MPI_INT, 0, MPI_COMM_WORLD);
        }
        //Non-0 ranks
        else{
            //Receive pair mapping
            MPI_Scatter(sbuf, 1, MPI_INT, &partner, 1, MPI_INT, 0, MPI_COMM_WORLD);
        }
        if(debug){
            if (partner == -1){
                cout << "Rank " << my_rank << ", Group " << my_aries_grp << " is not participating...\n";
            }
            else{
                cout << "Rank " << my_rank << ", Group " << my_aries_grp << " sending to rank " << partner << ".\n";
            }
        }

        //Rank 0 checks to see if benchmark has gone over time
        //If it goes past, it ibcasts telling all ranks to shutdown
        if (my_rank == 0){
            tstart = MPI_Wtime();  
            tend = MPI_Wtime();
            while ((tend - tstart) < DEADLINE){
                sleep(1);
                tend = MPI_Wtime();
            }
            stoptest = 1;
            MPI_Ibcast(&stoptest, 1, MPI_INT, 0,  MPI_COMM_WORLD, &endrequest);
        }

        else if(partner != -1){
            //Allocate and touch memory 
            if (posix_memalign((void**)&sdata, align, MSG_SIZE)) {
                cerr << "Error: posix_memalign\n";
                MPI_Finalize();
                exit(EXIT_FAILURE);
            }
            if (posix_memalign((void**)&rdata, align, MSG_SIZE)) {
                cerr << "Error: posix_memalign\n";
                MPI_Finalize();
                exit(EXIT_FAILURE);
            }
            memset(sdata, 's', MSG_SIZE);
            memset(rdata, 'r', MSG_SIZE);

            MPI_Ibcast(&stoptest, 1, MPI_INT, 0,  MPI_COMM_WORLD, &endrequest);
            tstart = MPI_Wtime();
            for (int loop = 0; loop < MAX_LOOPS; loop++){
                //I already let my partner know last iteration to stop
                if (sdata[0] == 'h'){
                    bw = MSG_SIZE / (1024*1024) * (loop-1) * WINDOW_SIZE;
                    if (debug) cout << "Loop " << loop << " Rank " << my_rank << " bytes transmitted " << bw << "\n";
                    break;
                }
                if (stoptest == 1){
                    if (debug) cout << "Loop " << loop << " Rank " << my_rank << " received stoptest bcast\n";
                    memset(sdata, 'h', MSG_SIZE);
                }
                for (int j = 0; j < WINDOW_SIZE; j++){
                    MPI_Irecv(rdata, MSG_SIZE, MPI_CHAR, partner, 100, MPI_COMM_WORLD, rrequest + j);
                }
                for (int j = 0; j < WINDOW_SIZE; j++){
                    MPI_Isend(sdata, MSG_SIZE, MPI_CHAR, partner, 100, MPI_COMM_WORLD, srequest + j);
                }
                MPI_Waitall(WINDOW_SIZE, srequest, sstat);
                MPI_Waitall(WINDOW_SIZE, rrequest, rstat);
                //Calculate bandwidth
                //Partner received message to stop and notified me
                if (rdata[0] == 'h'){
                    bw = MSG_SIZE / (1024*1024) * loop * WINDOW_SIZE;
                    if (debug) cout << "Loop " << loop << " Rank " << my_rank << " bytes transmitted " << bw << "\n";
                    break;
                }
            }
            if (stoptest == 0){
                bw = MSG_SIZE / (1024*1024) * MAX_LOOPS * WINDOW_SIZE;
            }
            tend = MPI_Wtime();
            t = tend - tstart;
            bw /= t;
            if (debug) cout << "Rank " << my_rank << " MBps " << bw << "\n";
            //Gather results back at root
            free(sdata);
            free(rdata);
        }
        MPI_Gather(&bw, 1, MPI_DOUBLE, &tbuf, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);  
    }
    if (my_rank == 0){
        //Write out the results
        auto t = time(nullptr);
        ostringstream oss;
        oss << "bidibiba_" << t << ".out";
        string fname = oss.str();
        ofstream results_file;
        results_file.open(fname);
        results_file << "Node1, Grp1, Node2, Grp2, BW(MiB/s)\n";
        // Each rank j (nodeid nbuf[j]) sends/recvs from rank (sbuf[j]) 
        // convert sbuf[j] to nodeid (nbuf)
        for (int j = 0; j < num_procs; j++){
            if(sbuf[j] != -1){
                results_file << nbuf[j] << ", " 
                    << nbuf[j]/384 << ", "
                    << nbuf[sbuf[j]] << ", "
                    << nbuf[sbuf[j]]/384 << ", "
                    << tbuf[j] << "\n";
            }
        }
        results_file.close();
        double t_sum, t_min, t_mean, t_50, t_75, t_max;
        vector <double> tvec(tbuf, tbuf + sizeof(tbuf)/sizeof(double));
        //Removing the values for ranks that didn't participlate
        
        tvec.erase(remove_if(tvec.begin(), tvec.end(), isZero), tvec.end());
        sort(tvec.begin(), tvec.end());
        cout << "A total of " << tvec.size() << " nodes participating from " 
            << groups.size() << " groups.\n"; 
        cout << "*****SUMMARY BW (MiB/s)*****\n";
        t_sum = accumulate(tvec.begin(), tvec.end(), 0);
        t_min = tvec[0];
        t_mean = t_sum/tvec.size();
        t_50 = tvec[(int)tvec.size()/2];
        t_75 = tvec[(int)tvec.size()*.75];
        t_max = tvec.back();
        cout << "min\tmean\t50\t75\tmax\tsum\n";
        cout << t_min << "\t"
            << t_mean << "\t"
            << t_50 << "\t"
            << t_75 << "\t"
            << t_max << "\t"
            << t_sum << "\n";
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
