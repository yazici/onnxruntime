#include <iostream>
#include "core/graph/model.h"
#include "core/graph/op.h"
#include "core/graph/utils.h"
#include "onnx/onnx_pb.h"
#include "gtest/gtest.h"

using namespace onnx;

namespace LotusIR {
namespace Test {
TEST(OpRegistrationTest, AffineOp) {
  auto op = OpSchemaRegistry::Schema("Affine");
  EXPECT_TRUE(nullptr != op);
  size_t input_size = op->inputs().size();
  EXPECT_EQ(input_size, 1);
  EXPECT_EQ(op->inputs()[0].GetTypes(), op->outputs()[0].GetTypes());
  size_t attr_size = op->attributes().size();
  EXPECT_EQ(attr_size, 2);
  auto attr_alpha = op->attributes().find("alpha")->second;
  EXPECT_EQ(attr_alpha.name, "alpha");
  EXPECT_EQ(attr_alpha.type, AttrType::AttributeProto_AttributeType_FLOAT);
  auto attr_beta = op->attributes().find("beta")->second;
  EXPECT_EQ(attr_beta.name, "beta");
  EXPECT_EQ(attr_beta.type, AttrType::AttributeProto_AttributeType_FLOAT);
}

TEST(OpRegistrationTest, EmbeddingOp) {
  auto op = OpSchemaRegistry::Schema("Embedding", kMSDomain);
  EXPECT_TRUE(nullptr != op);
  size_t input_size = op->inputs().size();
  EXPECT_EQ(input_size, 2);
  DataTypeSet input_types, output_types;
  input_types.emplace(Utils::DataTypeUtils::ToType("tensor(uint64)"));
  output_types.emplace(Utils::DataTypeUtils::ToType("tensor(float16)"));
  output_types.emplace(Utils::DataTypeUtils::ToType("tensor(float)"));
  output_types.emplace(Utils::DataTypeUtils::ToType("tensor(double)"));
  EXPECT_EQ(op->inputs()[0].GetTypes(), input_types);
  EXPECT_EQ(op->outputs()[0].GetTypes(), output_types);

  size_t attr_size = op->attributes().size();
  EXPECT_EQ(attr_size, 2);
  auto input_dim_attr = op->attributes().find("input_dim")->second;
  EXPECT_EQ(input_dim_attr.name, "input_dim");
  EXPECT_EQ(input_dim_attr.type, AttrType::AttributeProto_AttributeType_INT);
  auto output_dim_attr = op->attributes().find("output_dim")->second;
  EXPECT_EQ(output_dim_attr.name, "output_dim");
  EXPECT_EQ(output_dim_attr.type, AttrType::AttributeProto_AttributeType_INT);
}

TEST(FeatureVectorizerTest, TraditionalMlOpTest) {
  Model model("traditionalMl");
  auto graph = model.MainGraph();

  // Case: A traditional ml graph.
  //                           SouceNode
  //                              |
  //                       node_1(CastMap)
  //                      (tensor(float))
  //                             |
  //                    node_5 (FeatureVectorizer)
  //                              |
  //                           SinkNode

  std::vector<NodeArg *> inputs;
  std::vector<NodeArg *> outputs;

  // Type: tensor(float)
  TypeProto tensor_float;
  tensor_float.mutable_tensor_type()->set_elem_type(TensorProto::FLOAT);

  // Type: map(int64,float);
  TypeProto map_int64_float;
  auto map_type = map_int64_float.mutable_map_type();
  map_type->set_key_type(TensorProto::INT64);
  auto map_value_type = map_type->mutable_value_type()->mutable_tensor_type();
  map_value_type->set_elem_type(TensorProto::FLOAT);
  map_value_type->mutable_shape();

  NodeArg *input_arg1 = new NodeArg("node_1_in_1", &map_int64_float);
  inputs.clear();
  inputs.push_back(input_arg1);
  NodeArg *output_arg1 = new NodeArg("node_1_out_1", &tensor_float);
  outputs.clear();
  outputs.push_back(output_arg1);
  graph->AddNode("node_1", "CastMap", "node 1", inputs, outputs, nullptr, kMLDomain);

  inputs.clear();
  inputs.push_back(output_arg1);

  NodeArg *output_arg4 = new NodeArg("node_4_out_1", &tensor_float);
  outputs.clear();
  outputs.push_back(output_arg4);
  graph->AddNode("node_4", "FeatureVectorizer", "node 4", inputs, outputs, nullptr, kMLDomain);
  auto status = graph->Resolve();
  EXPECT_TRUE(status.IsOK());

  delete input_arg1;
  delete output_arg1;
  delete output_arg4;
}
}  // namespace Test
}  // namespace LotusIR
