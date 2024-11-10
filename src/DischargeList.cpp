#include "DischargeList.h"

#include <vector>
#include <iostream>
#include "Patient.h"

#include "arrow/io/file.h"
#include "parquet/stream_writer.h" 
#include "Reader_Writer.h"

DischargeList::DischargeList(){
    discharge_list = std::vector<Patient>();
}

DischargeList::DischargeList(std::string p){
    discharge_list = std::vector<Patient>();
    DischargeList::set_path(p);

    std::shared_ptr<arrow::io::FileOutputStream> outfile;

    std::cout << "Making outfile" << std::endl;
    std::cout << "Path: " << path << std::endl;

    PARQUET_ASSIGN_OR_THROW(
        outfile,
        arrow::io::FileOutputStream::Open(path));

    std::shared_ptr<parquet::schema::GroupNode> schema = SetupSchema();
    
    parquet::WriterProperties::Builder builder;
    builder.compression(parquet::Compression::GZIP);

    os = parquet::StreamWriter(parquet::ParquetFileWriter::Open(outfile, schema, builder.build()));
}

void DischargeList::add_patient(Patient patient){
    n_patients += 1;
    // discharge_list.push_back(patient);
    os << (patient.get_pathway()) << (patient.get_base_duration()) << (patient.get_arrival_t()) << (patient.get_first_appt())
        << (patient.get_n_appts()) << (patient.get_discharge_time()) << (patient.get_n_ext())
        << (patient.get_sojourn_time()) << (patient.get_total_wait_time()) << (patient.get_discharge_duration()) 
        << (patient.get_age_out()) << parquet::EndRow;
}

int DischargeList::get_n_patients(){return n_patients;}

int DischargeList::size(){
    return discharge_list.size();
}

// setter methods
void DischargeList::set_path(std::string p){path = p;}

// getter methods
std::vector<Patient> DischargeList::get_discharge_list(){return discharge_list;}