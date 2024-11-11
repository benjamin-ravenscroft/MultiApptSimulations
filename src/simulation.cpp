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

        // setup output stream for waitlist statistics
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
void Simulation::set_att_probs(double p[2][4]){
    for (int i = 0; i < 2; i++){
        double sum = 0;
        for (int j = 0; j < 4; j++){
            sum += p[i][j];
            att_probs[i][j] = sum;
        }
    }

    for (int i = 0; i < 2; i++){
        for (int j = 0; j < 4; j++){
            std::cout << att_probs[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

// Getter methods
double Simulation::get_arr_age(){
    double age = age_dstb(rng);
    if (0.5 < age < 2.5) {
        return 0.5;
    } else if (age < 0.5) {
        return 0.0;
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
        stream_waitlist(epoch);
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
    // std::string extension_protocols[3] = {"Standard", "Reduced Frequency", "Waitlist"};
    std::string extension_protocols[1] = {"Standard"};
    int n_epochs = 10000;
    int max_caseload = 1;
    double arr_lam = 10;
    std::vector<int> serv_path = {7, 10, 13};
    std::vector<double> wait_effects = {1.2, 1.2, 1.2};
    std::vector<double> modality_effects = {0.15, 0.0, -0.15};
    std::vector<double> probs = {0.33, 0.33, 0.33};
    std::vector<double> modality_policies = {0.5, 0.5, 0.5};
    double att_probs[2][4] = {
        {0.879,0.025,0.063,0.033},
        {0.836,0.047,0.067,0.049}
    };
    double max_ax_age = 3.0;
    int runs = 1;
    std::vector<double> age_params = {1.5, 1.0};

    // get number of clinicians from utilization
    // float utilization = 1;
    // int n_clinicians = utilization_to_servers(utilization, serv_path, probs, arr_lam);
    // std::cout << "Number of clinicians for utilization " << utilization << ": " << n_clinicians << std::endl;
    int n_clinicians = 80;
    std::vector<int> clinicians = {n_clinicians};

    // set priority ordering of waitlist
    std::vector<int> p_order = {0, 1, 2};
    bool priority_wlist = false;

    std::string folder = "/mnt/d/OneDrive - University of Waterloo/KidsAbility Research/Service Duration Analysis/C++ Simulations/";

    // parse command-line args if passed
    if (argc > 1) {
        if (argc == 11) {
            std::cout << "Processing with CLI arguments." << std::endl;
            n_epochs=std::stoi(argv[1]);
            arr_lam=std::stod(argv[2]);
            runs=std::atof(argv[9]);
            folder = argv[10];
        } else {
            throw std::invalid_argument("Parameter Arguments Invalid Size Error: Attempted to pass parameter arguments, but too few included. n_args: " + std::to_string(argc));
        }
    } else {
        folder += "test/";
    }

    std::vector<std::pair<std::string, double>> parameters{std::pair("n_epochs", n_epochs), std::pair("servers", clinicians[0]),
                                                            std::pair("runs", runs), std::pair("arr_lam", arr_lam)};
    write_csv(folder+"parameters.csv", parameters);

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
                                    dl, wl);
        sim.generate_servers();
        sim.run();
        std::cout << "N admitted: " << sim.get_n_admitted() << " N discharged: " << sim.get_n_discharged() << " N on waitlist: " << sim.get_n_waitlist() << std::endl;
    }
};