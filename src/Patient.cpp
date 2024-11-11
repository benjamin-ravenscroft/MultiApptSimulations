#include "Patient.h"
#include <iostream>

// To-Do List:
// 1. Implement passing max age

Patient::Patient(int arrival_time, double arrival_age, int pathway, int base_duration, 
                double wait_ext_beta, double modality_ext_beta, double modality_policy,
                const std::array<std::array<double, 4>, 2> (&att_probs),
                std::mt19937 &gen) : att_probs(att_probs){
    Patient::set_arrival_time(arrival_time);
    Patient::set_pathway(pathway);
    Patient::set_base_duration(base_duration);
    Patient::set_service_duration(base_duration);
    Patient::set_wait_ext_beta(wait_ext_beta);
    Patient::set_modality_ext_beta(modality_ext_beta);
    Patient::set_modality_policy(modality_policy);
    Patient::set_modality_dstb(std::uniform_real_distribution<>(0.0, 1.0));
    Patient::set_extended(0);
    rng = gen;
}

void Patient::add_appt(int epoch){
    appt_epochs.push_back(epoch);
}

void Patient::add_wait(int wlist_arr_t, int add_t){
    total_wait_time += (add_t - wlist_arr_t)/52;    // convert weeks to years
}

int Patient::check_attendance(int modality) {
    float prob = modality_dstb(rng);
    if (prob <= att_probs[modality][0]) {
        return 0;
    } else if (prob <= att_probs[modality][1] + att_probs[modality][2]) {
        return 1;
    } else {
        return 2;
    }
}

std::array<int, 2> Patient::process_patient(int epoch, int wl_len){
    // results returns a 2-element array
    // results[0] = 1 if patient is treated, 0 if not
    // results[1] = 1 if patient is discharged, 2 if aged-out, 0 if still in service, -1 if not treated
    int modality = 0;
    if (modality_dstb(rng) > modality_policy) {
        modality = 1;
    }
    int att = Patient::check_attendance(modality);
    int check = 0;

    switch (att) {
        case 0:
            Patient::add_appt(epoch);
            check = Patient::check_complete(epoch);
            return std::array<int, 2> {1, check};
        case 1:
            check = Patient::check_complete(epoch);
            return std::array<int, 2> {1, check};
        case 2:
            check = Patient::check_complete(epoch);
            return std::array<int, 2> {0, check};
    }
}

// calculate the effect of waiting on service duration
double Patient::calculate_wait_effect(){
    return wait_ext_beta*total_wait_time;
}

// add the wait time effect to service duration
void Patient::add_wait_effect(){
    base_duration += calculate_wait_effect();
}

// calculate the proportion of in-person visits
double Patient::calculate_modality_effect(){
    return modality_effect*modality_sum/appt_epochs.size();
}

void Patient::add_modality_effect(){
    service_duration = base_duration + calculate_modality_effect();
}

// check if the patient has completed their service
// To-Do: Implement passing max age
int Patient::check_complete(int epoch){
    if (appt_epochs.size() >= service_duration){
        set_discharge_duration(service_duration);
        return 1;
    } else if (get_age(epoch) > 4.0) {
        set_age_out(1);
        return 2;
    } else {
        return 0;
    }
}

// member variable setters
void Patient::set_arrival_time(int t){arrival_time=t;}
void Patient::set_arrival_age(double a){arrival_age=a;}
void Patient::set_pathway(int p){pathway=p;}
void Patient::set_base_duration(int d){base_duration=d;}
void Patient::set_service_duration(int d){service_duration=d;}
void Patient::set_base_ext_p(double p){base_ext_p=p;}
void Patient::set_wait_ext_beta(double b){wait_ext_beta=b;}
void Patient::set_modality_ext_beta(double b){queue_ext_beta=b;}
void Patient::set_discharge_time(int epoch){discharge_time = epoch;}
void Patient::set_extended(int c){extended=c;}
void Patient::set_discharge_duration(int d){discharge_duration=d;}
void Patient::set_modality_effect(double m){modality_effect=m;}
void Patient::set_age_out(int a){age_out=a;}
void Patient::set_modality_policy(double p){modality_policy=p;}
void Patient::set_modality_dstb(std::uniform_real_distribution<> dstb){modality_dstb=dstb;}

// extraneous get-set methods
int Patient::get_pathway(){return pathway;}

int Patient::get_base_duration(){return base_duration;}

int Patient::get_arrival_t(){return arrival_time;}

double Patient::get_age(int epoch){
    return arrival_age + (epoch - arrival_time)/52;    // convert weeks to years
}

int Patient::get_first_appt(){
    if (appt_epochs.size() == 0){
        return -1;
    }
    return appt_epochs[0];
}

int Patient::get_n_appts(){return appt_epochs.size();}

int Patient::get_n_ext(){return extended;}

int Patient::get_discharge_time(){return discharge_time;}

int Patient::get_sojourn_time(){return discharge_time - arrival_time;}

int Patient::get_total_wait_time(){
    return total_wait_time;
}

int Patient::get_discharge_duration(){return discharge_duration;}

int Patient::get_age_out(){return age_out;}

double Patient::get_pct_face(){
    if (appt_epochs.size() == 0) {
        return 0.0;
    }
    return modality_sum/appt_epochs.size();
}