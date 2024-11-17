#include <iostream>
#include <cmath>
#include <random>
#include <map>
#include <iomanip>
#include <string>
#include <chrono>
#include <charconv>
#include <stdexcept>

#include "arrow/io/file.h"
#include "parquet/stream_writer.h"
#include <cxxopts.hpp>

#include "Simulation.h"
#include "Patient.h"
#include "Waitlist.h"
#include "DischargeList.h"
#include "Server.h"
#include "Reader_Writer.h"
#include "WriteCSV.h"

std::random_device rd;
std::mt19937 rng(rd());

Simulation::Simulation(int n_epochs, std::vector<int> clinicians, 
                        int max_caseload, double arr_lam,
                        std::vector<int> pathways, std::vector<double> wait_effects,
                        std::vector<double> modality_effects, std::vector<double> modality_policies,
                        double att_probs[2][4],
                        std::vector<double> probs, std::vector<double> age_params, 
                        double max_ax_age, std::string wl_path,
                        bool waitlist_logging,
                        DischargeList& dl, Waitlist& wl) : dl(dl), wl(wl) {

        Simulation::set_n_epochs(n_epochs);
        Simulation::set_clinicians(clinicians);
        Simulation::set_max_caseload(max_caseload);
        Simulation::set_arr_lam(arr_lam);
        Simulation::set_pathways(pathways);
        Simulation::set_wait_effects(wait_effects);
        Simulation::set_modality_effects(modality_effects);
        Simulation::set_modality_policies(modality_policies);
        Simulation::set_n_classes(pathways.size());
        Simulation::set_class_dstb(std::discrete_distribution<> (probs.begin(), probs.end()));
        Simulation::set_age_dstb(std::normal_distribution<> (age_params[0], age_params[1]));
        Simulation::set_att_probs(att_probs);
        Simulation::set_waitlist_logging(waitlist_logging);

        // setup output stream for waitlist statistics
        if (waitlist_logging) {
            std::cout << "Setting up waitlist output stream" << std::endl;
            std::shared_ptr<arrow::io::FileOutputStream> outfile;
            PARQUET_ASSIGN_OR_THROW(
                outfile,
                arrow::io::FileOutputStream::Open(wl_path)
            );
            std::cout << "Opened output file" << std::endl;
            std::shared_ptr<parquet::schema::GroupNode> schema = SetupSchema_Waitlist();
            parquet::WriterProperties::Builder builder;
            builder.compression(parquet::Compression::GZIP);
            wl_os = parquet::StreamWriter(parquet::ParquetFileWriter::Open(outfile, schema, builder.build()));
            std::cout << "Setup waitlist output stream" << std::endl;
        }
}

// Setter methods
void Simulation::set_n_epochs(int n){n_epochs = n;}
void Simulation::set_clinicians(std::vector<int> c){clinicians = c;}
void Simulation::set_max_caseload(int m){max_caseload = m;}
void Simulation::set_arr_lam(double l){arr_lam = l;}
void Simulation::set_pathways(std::vector<int> ps){pathways = ps;}
void Simulation::set_wait_effects(std::vector<double> ws){wait_effects = ws;}
void Simulation::set_modality_effects(std::vector<double> ms){modality_effects = ms;}
void Simulation::set_modality_policies(std::vector<double> ps){modality_policies = ps;}
void Simulation::set_probs(std::vector<double> ps){probs = ps;}
void Simulation::set_n_classes(int n){n_classes = n;}
void Simulation::set_class_dstb(std::discrete_distribution<> dstb){class_dstb = dstb;}
void Simulation::set_age_dstb(std::normal_distribution<> dstb){age_dstb = dstb;}
void Simulation::set_waitlist_logging(bool wl){waitlist_logging = wl;}
void Simulation::set_att_probs(double p[2][4]){
    for (int i = 0; i < 2; i++){
        double sum = 0;
        for (int j = 0; j < 4; j++){
            sum += p[i][j];
            att_probs[i][j] = sum;
        }
    }
}

// Getter methods
double Simulation::get_arr_age(){
    double age = age_dstb(rng);
    // std::cout << "Age: " << age << std::endl;
    if ((0.5 <= age) & (age <= 2.5)) {
        return age;
    } else if (age < 0.5) {
        return 0.25;
    } else {
        return 2.5;
    }
}

void Simulation::generate_servers() {
    for (int i = 0; i < clinicians.size(); i++) {
        for (int j = 0; j < clinicians[i]; j++) {
            servers.push_back(Server(max_caseload, wl, dl));
        }
    }
}

void Simulation::generate_arrivals(int epoch) {
    std::poisson_distribution<> arr_dstb(arr_lam);
    int n_patients = arr_dstb(rng);
    n_admitted += n_patients;
    // std::cout << "Adding " << n_patients << " patients to waitlist." << std::endl;
    for (int i = 0; i < n_patients; i++) {
        int pat_class = class_dstb(rng); // get int pat class
        double arr_age = get_arr_age();
        Patient pat = Patient(epoch, arr_age, pat_class, pathways[pat_class],
                            wait_effects[pat_class], modality_effects[pat_class],
                            modality_policies[pat_class], att_probs,
                            rng);
        wl.add_patient(pat, epoch);
        // std::cout << "Waitlist length after adding patient: " << wl.len_waitlist() << std::endl;
    }
    // std::cout << "successfuly generated arrivals" << std::endl;
}

void Simulation::run() {
    auto start = std::chrono::high_resolution_clock::now();
    for (int epoch = 0; epoch < n_epochs; epoch++) {
        generate_arrivals(epoch);
        for (int i = 0; i < servers.size(); i++) {
            servers[i].process_epoch(epoch);
        }
        if (waitlist_logging){stream_waitlist(epoch);}
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
    std::cout << "Simulation duration: " << duration.count() << "s." << std::endl;
}

int Simulation::get_n_admitted(){return n_admitted;}

int Simulation::get_n_discharged(){return dl.get_n_patients();}

int Simulation::get_n_waitlist(){return wl.len_waitlist();}

// not in use currently
void Simulation::write_statistics(std::string path){}

// deprecated for streaming option
void Simulation::write_parquet(std::string path) {
    std::shared_ptr<arrow::io::FileOutputStream> outfile;

    std::cout << "Making outfile" << std::endl;

    PARQUET_ASSIGN_OR_THROW(
        outfile,
        arrow::io::FileOutputStream::Open(path));

    std::cout << "Setting up schema" << std::endl;

    std::shared_ptr<parquet::schema::GroupNode> schema = SetupSchema();

    std::cout << "Schema setup" << std::endl;
    
    parquet::WriterProperties::Builder builder;
    builder.compression(parquet::Compression::GZIP);

    std::cout << "Initialized builder" << std::endl;

    parquet::StreamWriter os{
        parquet::ParquetFileWriter::Open(outfile, schema, builder.build())};

    std::cout << "Setup output stream" << std::endl;

    for (auto & patient : dl.get_discharge_list()) {
        os << (patient.get_pathway()) << (patient.get_base_duration()) << (patient.get_arrival_t()) 
            << (patient.get_first_appt()) << (patient.get_n_appts()) << (patient.get_discharge_time()) 
            << (patient.get_n_ext()) << (patient.get_sojourn_time()) << (patient.get_total_wait_time()) 
            << parquet::EndRow;
    }
}

void Simulation::stream_waitlist(int epoch){
    wl_os << epoch << wl.len_waitlist() << parquet::EndRow;
}

// functions for main
int utilization_to_servers(float utilization, std::vector<int> pathways,
                            std::vector<double> probs, double arr_lam){
    float mu = 0;
    for (int i = 0; i < pathways.size(); i++){
        mu += probs[i]*arr_lam*pathways[i];
    }
    return ceil(mu/utilization);
}

int main(int argc, char *argv[]){
    // std::string folder = "/mnt/d/OneDrive - University of Waterloo/KidsAbility Research/Service Duration Analysis/C++ Simulations/";

    // setup options parsing
    cxxopts::Options options("Service Duration Simulation", "Simulate service duration for multi-class, multi-server queueing system.");
    options.add_options()
        ("n,n_epochs", "Number of epochs", cxxopts::value<int>()->default_value("10000"))
        ("c,clinicians", "Number of clinicians", cxxopts::value<std::vector<int>>()->default_value("80"))
        ("m,max_caseload", "Maximum caseload per clinician", cxxopts::value<int>()->default_value("1"))
        ("a,arr_lam", "Arrival rate lambda", cxxopts::value<double>()->default_value("10"))
        ("f,folder", "Output folder", cxxopts::value<std::string>()->default_value("test/"))
        ("p,pathways", "Class pathways", cxxopts::value<std::vector<int>>()->default_value("7,10,13"))
        ("w,wait_effects", "Wait time effects", cxxopts::value<std::vector<double>>()->default_value("0.6,0.6,0.6"))
        ("e,modality_effects", "Modality effects", cxxopts::value<std::vector<double>>()->default_value("0.5,0.0,-0.5"))
        ("o,modality_policies", "Modality policies", cxxopts::value<std::vector<double>>()->default_value("0.5,0,1"))
        ("x,max_ax_age", "Maximum age for ax", cxxopts::value<double>()->default_value("3.0"))
        ("g,age_params", "Age parameters", cxxopts::value<std::vector<double>>()->default_value("1.5,1.0"))
        ("priority_order", "Priority order of waitlist", cxxopts::value<std::vector<int>>()->default_value("0,1,2"))
        ("priority_wlist", "Priority waitlist", cxxopts::value<bool>()->default_value("true"))
        ("arrival_probs", "Arrival probabilities", cxxopts::value<std::vector<double>>()->default_value("0.33,0.33,0.33"))
        ("r,runs", "Number of runs", cxxopts::value<int>()->default_value("1"))
        ("waitlist_log", "Log waitlist statistics", cxxopts::value<bool>()->default_value("false"))
        ("virtual_att_probs", "Attendance probabilities for virtual appointments", cxxopts::value<std::vector<double>>()->default_value("0.9,0.025,0.025,0.05"))
        ("face_att_probs", "Attendance probabilities for in person appointments", cxxopts::value<std::vector<double>>()->default_value("0.8,0.05,0.05,0.1"))
    ;

    auto result = options.parse(argc, argv);

    int n_epochs = result["n_epochs"].as<int>();
    std::vector<int> clinicians = result["clinicians"].as<std::vector<int>>();
    int max_caseload = result["max_caseload"].as<int>();
    double arr_lam = result["arr_lam"].as<double>();
    std::vector<double> probs = result["arrival_probs"].as<std::vector<double>>();
    // folder += result["folder"].as<std::string>();    // deprecated
    std::string folder = result["folder"].as<std::string>();
    std::vector<int> serv_path = result["pathways"].as<std::vector<int>>();
    std::vector<double> wait_effects = result["wait_effects"].as<std::vector<double>>();
    std::vector<double> modality_effects = result["modality_effects"].as<std::vector<double>>();
    std::vector<double> modality_policies = result["modality_policies"].as<std::vector<double>>();
    double max_ax_age = result["max_ax_age"].as<double>();
    std::vector<double> age_params = result["age_params"].as<std::vector<double>>();
    std::vector<int> p_order = result["priority_order"].as<std::vector<int>>();
    bool priority_wlist = result["priority_wlist"].as<bool>();
    int runs = result["runs"].as<int>();
    bool waitlist_logging = result["waitlist_log"].as<bool>();
    std::vector<double> virtual_att_probs = result["virtual_att_probs"].as<std::vector<double>>();
    std::vector<double> face_att_probs = result["face_att_probs"].as<std::vector<double>>();

    // set cancellation likelihoods
    // double att_probs[2][4] = {
    //     {0.9,0.025,0.025,0.05},
    //     {0.8,0.05,0.05,0.10}
    // };
    double att_probs[2][4] = {0};
    for (int i = 0; i < 4; i++) {
        att_probs[0][i] = virtual_att_probs[i];
    }
    for (int i = 0; i < 4; i++) {
        att_probs[1][i] = face_att_probs[i];
    }

    // create output paths
    std::string path = folder;
    std::string wl_path = folder + "waitlist_data/";

    for (int run = 0; run < runs; run++){
        std::cout << "Run " << run << std::endl;
        std::string run_path =  path + ("simulation_data_" + std::to_string(run) + ".parquet");
        std::string waitlist_path = wl_path + ("waitlist_data_" + std::to_string(run) + ".parquet");
        // initialize waitlist and discharge list instances
        DischargeList dl = DischargeList(run_path);
        Waitlist wl = Waitlist(serv_path.size(), max_ax_age,
                                priority_wlist, p_order,
                                rng, dl);
        Simulation sim = Simulation(n_epochs, clinicians, 
                                    max_caseload, arr_lam,
                                    serv_path, wait_effects, 
                                    modality_effects, modality_policies,
                                    att_probs,
                                    probs, age_params, 
                                    max_ax_age, waitlist_path,
                                    waitlist_logging,
                                    dl, wl);
        sim.generate_servers();
        sim.run();
        std::cout << "N admitted: " << sim.get_n_admitted() << " N discharged: " << sim.get_n_discharged() << " N on waitlist: " << sim.get_n_waitlist() << std::endl;
    }
};