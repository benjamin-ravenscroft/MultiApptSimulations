#include "Server.h"
#include <deque>
#include "Patient.h"
#include "Waitlist.h"
#include "DischargeList.h"

Server::Server(Waitlist &wl, DischargeList &dl) : waitlist(wl), discharge_list(dl) {}

void Server::add_patient(Patient &patient) {
    caseload.push_back(patient);
    n_patients += 1;
    if (caseload.size() > n_patients){
        std::cout << "Caseload size: " << caseload.size() << " n_patients: " << n_patients << std::endl;
        throw std::runtime_error("Error - Line 13");
    }
}

void Server::add_from_waitlist(int epoch){
    if (waitlist.check_availability(epoch)) {
        std::pair<Patient, int> pair = waitlist.get_patient(epoch);
        pair.first.add_wait(pair.second, epoch);
        add_patient(pair.first);
    } else if (logging) {
        std::cout << "No patients on waitlist to add." << std::endl;
    }
}

void Server::process_extension(Patient patient, int epoch){
    waitlist.add_patient(patient, epoch);
    n_patients -= 1;
}

void Server::process_epoch(int epoch){
    // std::cout << "Processing epoch. n_patients: " << n_patients << " Caseload len: " << caseload.size() << std::endl;
    if (n_patients == 0) {
        if (caseload.size() > n_patients) {
            std::cout << "Caseload size: " << caseload.size() << " n_patients: " << n_patients << std::endl;
            throw std::runtime_error("Error - Line 38");
        }
        add_from_waitlist(epoch);
    } else {
        Patient p = caseload.front();
        caseload.pop_front();
        int check = p.process_patient(epoch, waitlist.len_waitlist());
        if (check == 1 | check == 2) { // if they have reached their service_max
            p.set_discharge_time(epoch);
            discharge_list.add_patient(p);
            n_patients -= 1;
        } else {
            caseload.push_back(p);
            if (caseload.size() > n_patients) {
                std::cout << "Caseload size: " << caseload.size() << " n_patients: " << n_patients << std::endl;
                throw std::runtime_error("Error - Line 84");
            }
        }
    }
}

// member variable setter methods
void Server::set_max_caseload(int max){max_caseload=max;}

// logging methods
void Server::print_patients() {
    if (caseload.size() > 0) {
        for (auto p : caseload) {
            std::cout << p.get_total_wait_time();
        }
    } else if (logging) {
        std::cout << "No patients on caseload.";
    }
}