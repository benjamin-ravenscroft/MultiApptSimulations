#ifndef READER_WRITER_H
#define READER_WRITER_H

#include <arrow/io/file.h>
#include <arrow/util/logging.h>

#include <parquet/api/reader.h>
#include <parquet/api/writer.h>

using parquet::LogicalType;
using parquet::Repetition;
using parquet::Type;
using parquet::schema::GroupNode;
using parquet::schema::PrimitiveNode;

static std::shared_ptr<GroupNode> SetupSchema() {
    parquet::schema::NodeVector fields;

    fields.push_back(PrimitiveNode::Make("class", Repetition::REQUIRED,
                                        Type::INT32, parquet::ConvertedType::INT_32));

    fields.push_back(PrimitiveNode::Make("base_duration", Repetition::REQUIRED,
                                        Type::INT32, parquet::ConvertedType::INT_32));

    fields.push_back(PrimitiveNode::Make("arrival_t", Repetition::REQUIRED,
                                        Type::INT32, parquet::ConvertedType::INT_32));

    fields.push_back(PrimitiveNode::Make("first_appt", Repetition::REQUIRED,
                                        Type::INT32, parquet::ConvertedType::INT_32));

    fields.push_back(PrimitiveNode::Make("n_appts", Repetition::REQUIRED,
                                        Type::INT32, parquet::ConvertedType::INT_32));

    fields.push_back(PrimitiveNode::Make("discharge_t", Repetition::REQUIRED,
                                        Type::INT32, parquet::ConvertedType::INT_32));

    fields.push_back(PrimitiveNode::Make("n_ext", Repetition::REQUIRED,
                                        Type::INT32, parquet::ConvertedType::INT_32));

    fields.push_back(PrimitiveNode::Make("sojourn_time", Repetition::REQUIRED,
                                        Type::INT32, parquet::ConvertedType::INT_32));

    fields.push_back(PrimitiveNode::Make("total_wait_time", Repetition::REQUIRED,
                                        Type::INT32, parquet::ConvertedType::INT_32));

    fields.push_back(PrimitiveNode::Make("discharge_duration", Repetition::REQUIRED,
                                        Type::INT32, parquet::ConvertedType::INT_32));

    fields.push_back(PrimitiveNode::Make("pct_face", Repetition::REQUIRED,
                                        Type::DOUBLE, parquet::ConvertedType::NONE)); 

    fields.push_back(PrimitiveNode::Make("age_out", Repetition::REQUIRED,
                                        Type::INT32, parquet::ConvertedType::INT_32));

    return std::static_pointer_cast<GroupNode>(
        GroupNode::Make("schema", Repetition::REQUIRED, fields));
        
}

static std::shared_ptr<GroupNode> SetupSchema_Waitlist() {
    parquet::schema::NodeVector fields;

    fields.push_back(PrimitiveNode::Make("epoch", Repetition::REQUIRED,
                                        Type::INT32, parquet::ConvertedType::INT_32));

    fields.push_back(PrimitiveNode::Make("waitlist_len", Repetition::REQUIRED,
                                        Type::INT32, parquet::ConvertedType::INT_32));

    return std::static_pointer_cast<GroupNode>(
        GroupNode::Make("schema", Repetition::REQUIRED, fields));
        
}
#endif