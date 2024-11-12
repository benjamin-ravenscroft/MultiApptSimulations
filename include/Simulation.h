#ifndef SIMULATION_H
#define SIMULATION_H

#include <vector>
#include <random>
#include "Waitlist.h"
#include "DischargeList.h"
#include "Server.h"

class Simulation{
    public:
        Simulation(int n_epochs, std::vector<int> clinicians,
                int max_caseload, double arr_lam, 
                std::vector<int> pathways, std::vector<double> wait_effects, 
                std::vector<double> modality_effects, std::vector<double> modality_policies,
                double att_probs[2][4],
                std::vector<double> probs, std::vector<double> age_params,
                double max_ax_age, std::string wl_path,
                bool waitlist_logging,
                DischargeList& dl, Waitlist& wl);
        
        void generate_servers();
        void generate_arrivals(int epoch);
        void run();
        void write_parquet(std::string path);
        void write_statistics(std::string path);

        int get_n_admitted();

        int get_n_discharged();
        int get_n_waitlist();

        // member-variable setters
        void set_n_epochs(int n_epochs);
        void set_clinicians(std::vector<int> clin);
        void set_max_caseload(int max_caseload);
        void set_arr_lam(double arr_lam);
        void set_pathways(std::vector<int> pathways);
        void set_wait_effects(std::vector<double> wait_effects);
        void set_modality_effects(std::vector<double> modality_effects);
        void set_modality_policies(std::vector<double> modality_policies);
        void set_probs(std::vector<double> probs);
        void set_n_classes(int n_classes);
        void set_class_dstb(std::discrete_distribution<> class_dstb);
        void set_age_dstb(std::normal_distribution<> age_dstb);
        void set_att_probs(double probs[2][4]);
        void set_waitlist_logging(bool waitlist_logging);
        // void set_discharge_list(std::string path);
        // void set_waitlist(int n_classes, std::mt19937 &gen, double max_ax_age, DischargeList &dl);
        void stream_waitlist(int epoch);

        double get_arr_age();
        
    private:
        int n_epochs;
        std::vector<int> clinicians;
        int max_caseload;
        double arr_lam;
        std::vector<int> pathways; // map the integer key to string labels for classes outside of the simulation
        std::vector<double> wait_effects;   // wait time effect for each class
        std::vector<double> modality_effects; // modality effect for each class
        std::vector<double> modality_policies; // modality policy for each class
        std::array<std::array<double, 4>, 2> att_probs; // arrival probabilities for each class
        std::vector<double> probs;  // arrival probabilities for each class
        double max_ax_age;
        int n_classes;
        Waitlist& wl;
        DischargeList& dl;
        int n_admitted = 0;
        std::vector<Server> servers;
        std::discrete_distribution<> class_dstb;
        std::normal_distribution<> age_dstb;
        parquet::StreamWriter wl_os;
        bool waitlist_logging = false;
};
#endif
