#include <iostream>
#ifdef _MSC_VER
#pragma warning(push)
// 'identifier' : unreferenced formal parameter
#pragma warning(disable : 4100)
// 'type' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable : 4800)
#endif
#include "google/protobuf/util/message_differencer.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "core/graph/graph.h"
#include "core/graph/model.h"
#include "core/graph/op.h"
#include "gtest/gtest.h"
#include "test/ir/node_helper.h"

namespace LotusIR {
namespace Test {
using google::protobuf::util::MessageDifferencer;

TEST(GraphTraversalTest, ReverseDFS) {
  ONNX_OPERATOR_SCHEMA(Variable_DFS)
      .SetDoc("Input variable.")
      .Input(0, "input_1", "docstr for input_1.", "tensor(int32)")
      .Output(0, "output_1", "docstr for output_1.", "tensor(int32)");
  ONNX_OPERATOR_SCHEMA(Add_DFS)
      .SetDoc("Add two integers.")
      .Input(0, "input_1", "docstr for input_1.", "tensor(int32)")
      .Input(1, "input_2", "docstr for input_2.", "tensor(int32)")
      .Output(0, "output_1", "docstr for output_1.", "tensor(int32)");
  ONNX_OPERATOR_SCHEMA(NoOp_DFS)
      .SetDoc("Operator doing nothing.")
      .Input(0, "input_1", "docstr for input_1.", "tensor(int32)")
      .Output(0, "output_1", "docstr for output_1.", "tensor(int32)");

  Model model("graph_1");
  auto &graph = *(model.MainGraph());

  // Case 1: A normal graph.
  //                 SouceNode
  //                 /       \
  //  node_1 (Variable)      node_2 (Variable)
  //                 \       /
  //                 node_3 (Add)
  //                     |
  //                 node_4 (NoOp)
  //                     |
  //                  SinkNode
  std::vector<NodeArg *> inputs;
  std::vector<NodeArg *> outputs;

  TypeProto tensor_int32;
  tensor_int32.mutable_tensor_type()->set_elem_type(TensorProto_DataType_INT32);
  tensor_int32.mutable_tensor_type()->mutable_shape()->add_dim()->set_dim_value(1);

  NodeArg *input_arg = new NodeArg("node_1_in_1", &tensor_int32);
  inputs.push_back(input_arg);
  NodeArg *output_arg = new NodeArg("node_1_out_1", &tensor_int32);
  outputs.push_back(output_arg);
  graph.AddNode("node_1", "Variable_DFS", "node 1", inputs, outputs);

  NodeArg *input_arg2 = new NodeArg("node_2_in_1", &tensor_int32);
  inputs.clear();
  inputs.push_back(input_arg2);
  NodeArg *output_arg2 = new NodeArg("node_2_out_1", &tensor_int32);
  outputs.clear();
  outputs.push_back(output_arg2);
  graph.AddNode("node_2", "Variable_DFS", "node 2", inputs, outputs);

  inputs.clear();
  inputs.push_back(output_arg);
  inputs.push_back(output_arg2);
  NodeArg *output_arg3 = new NodeArg("node_3_out_1", &tensor_int32);
  outputs.clear();
  outputs.push_back(output_arg3);
  graph.AddNode("node_3", "Add_DFS", "node 3", inputs, outputs);

  inputs.clear();
  inputs.push_back(output_arg3);
  NodeArg *output_arg4 = new NodeArg("node_4_out_1", &tensor_int32);
  outputs.clear();
  outputs.push_back(output_arg4);
  graph.AddNode("node_4", "NoOp_DFS", "node 4", inputs, outputs);
  auto status = graph.Resolve();
  EXPECT_TRUE(status.IsOK());

  std::vector<const Node *> from;
  from.push_back(graph.SinkNode());

  std::vector<std::string> enter_leave_sequence;

  struct NodeCompareName {
    bool operator()(const Node *n1, const Node *n2) const {
      return n1->Name() < n2->Name();
    }
  };

  graph.ReverseDFSFrom(from,
                       [&enter_leave_sequence](const Node *n) {
                         std::string s("enter:");
                         s += n->Name();
                         enter_leave_sequence.push_back(s);
                       },
                       [&enter_leave_sequence](const Node *n) {
                         std::string s("leave:");
                         s += n->Name();
                         enter_leave_sequence.push_back(s);
                       },
                       NodeCompareName());

  /*for (size_t i = 0; i < enter_leave_sequence.size(); i++) {
        printf("%s\n", enter_leave_sequence[i].c_str());
    }*/

  EXPECT_EQ(enter_leave_sequence.size(), 12);
  EXPECT_EQ("enter:_Graph_Sink", enter_leave_sequence[0]);
  EXPECT_EQ("enter:node_4", enter_leave_sequence[1]);
  EXPECT_EQ("enter:node_3", enter_leave_sequence[2]);
  EXPECT_EQ("enter:node_2", enter_leave_sequence[3]);
  EXPECT_EQ("enter:_Graph_Source", enter_leave_sequence[4]);
  EXPECT_EQ("leave:_Graph_Source", enter_leave_sequence[5]);
  EXPECT_EQ("leave:node_2", enter_leave_sequence[6]);
  EXPECT_EQ("enter:node_1", enter_leave_sequence[7]);
  EXPECT_EQ("leave:node_1", enter_leave_sequence[8]);
  EXPECT_EQ("leave:node_3", enter_leave_sequence[9]);
  EXPECT_EQ("leave:node_4", enter_leave_sequence[10]);
  EXPECT_EQ("leave:_Graph_Sink", enter_leave_sequence[11]);
}

TEST(ResolvingGraphTest, GraphConstruction_VerifyNoDuplicateName) {
  Model model("graph_1");
  auto graph = model.MainGraph();

  EXPECT_EQ("graph_1", graph->Name());

  std::vector<NodeArg *> inputs;
  std::vector<NodeArg *> outputs;

  // INT32 vector.
  TypeProto output_type;
  output_type.mutable_tensor_type()->set_elem_type(TensorProto_DataType_INT32);
  output_type.mutable_tensor_type()->mutable_shape()->add_dim()->set_dim_value(1);

  NodeArg *output_arg = new NodeArg("node_1_out_1", &output_type);
  outputs.push_back(output_arg);
  graph->AddNode("node_1", "Variable", "node 1.", inputs, outputs);

  // Case 1: Adding two nodes with same node name should fail.
  auto node_with_dup_name = graph->AddNode("node_1", "Variable", "node 2", inputs, outputs);
  auto status = graph->Resolve();
  EXPECT_FALSE(status.IsOK());
  EXPECT_EQ("Error: two nodes with same node name (node_1).", status.ErrorMessage());
  graph->RemoveNode(node_with_dup_name->Index());

  // Case 2: Adding two nodes with same output arg name should fail.
  graph->AddNode("node_2", "Variable", "node 2", inputs, outputs);
  status = graph->Resolve();
  EXPECT_FALSE(status.IsOK());
  EXPECT_EQ("Error: two output args with same name (node_1_out_1).", status.ErrorMessage());
  //delete output_arg;
}

TEST(ResolvingGraphTest, GraphConstruction_VerifyNodeAndOpMatch) {
  Model model("graph_1");
  auto graph = model.MainGraph();

  std::vector<NodeArg *> inputs;
  std::vector<NodeArg *> outputs;

  // INT32 vector.
  TypeProto output_type;
  output_type.mutable_tensor_type()->set_elem_type(TensorProto_DataType_INT32);
  output_type.mutable_tensor_type()->mutable_shape()->add_dim()->set_dim_value(1);

  NodeArg *output_arg = new NodeArg("node_1_out_1", &output_type);
  outputs.push_back(output_arg);
  // Case: Adding node refering to non-existing operator should fail.
  graph->AddNode("node_1", "OpNotExist", "node 1", inputs, outputs);
  auto status = graph->Resolve();
  EXPECT_FALSE(status.IsOK());
  EXPECT_EQ(
      "Error: the operator or function (OpNotExist) referred to by node (node_1) does not exist.",
      status.ErrorMessage());

  //delete output_arg;
}

TEST(ResolvingGraphTest, GraphConstruction_CheckIsAcyclic) {
  ONNX_OPERATOR_SCHEMA(Variable_Fake)
      .SetDoc("Input variable.")
      .Input(0, "input_1", "docstr for input_1.", "tensor(int32)")
      .Output(0, "output_1", "docstr for output_1.", "tensor(int32)");
  ONNX_OPERATOR_SCHEMA(Add_Fake)
      .SetDoc("Add two integers.")
      .Input(0, "input_1", "docstr for input_1.", "tensor(int32)")
      .Input(1, "input_2", "docstr for input_2.", "tensor(int32)")
      .Output(0, "output_1", "docstr for output_1.", "tensor(int32)");
  ONNX_OPERATOR_SCHEMA(NoOp_Fake)
      .SetDoc("Operator doing nothing.")
      .Input(0, "input_1", "docstr for input_1.", "tensor(int32)")
      .Output(0, "output_1", "docstr for output_1.", "tensor(int32)");

  Model model("graph_1");
  auto &graph = *(model.MainGraph());

  // Case 1: A normal graph.
  //                 SouceNode
  //                 /       \
            //  node_1 (Variable)      node_2 (Variable)
  //                 \       /
  //                 node_3 (Add)
  //                     |
  //                 node_4 (NoOp)
  //                     |
  //                  SinkNode
  std::vector<NodeArg *> inputs;
  std::vector<NodeArg *> outputs;

  std::unordered_map<std::string, std::pair<std::vector<NodeArg *>, std::vector<NodeArg *>>>
      expected_node_name_to_input_output_args;

  TypeProto tensor_int32;
  tensor_int32.mutable_tensor_type()->set_elem_type(TensorProto_DataType_INT32);
  tensor_int32.mutable_tensor_type()->mutable_shape()->add_dim()->set_dim_value(1);

  NodeArg *input_arg = new NodeArg("node_1_in_1", &tensor_int32);
  inputs.push_back(input_arg);
  NodeArg *output_arg = new NodeArg("node_1_out_1", &tensor_int32);
  outputs.push_back(output_arg);
  expected_node_name_to_input_output_args["node_1"] = {inputs, outputs};
  auto node_1 = graph.AddNode("node_1", "Variable_Fake", "node 1", inputs, outputs);

  NodeArg *input_arg2 = new NodeArg("node_2_in_1", &tensor_int32);
  inputs.clear();
  inputs.push_back(input_arg2);
  NodeArg *output_arg2 = new NodeArg("node_2_out_1", &tensor_int32);
  outputs.clear();
  outputs.push_back(output_arg2);
  expected_node_name_to_input_output_args["node_2"] = {inputs, outputs};
  graph.AddNode("node_2", "Variable_Fake", "node 2", inputs, outputs);

  inputs.clear();
  inputs.push_back(output_arg);
  inputs.push_back(output_arg2);
  NodeArg *output_arg3 = new NodeArg("node_3_out_1", &tensor_int32);
  outputs.clear();
  outputs.push_back(output_arg3);
  expected_node_name_to_input_output_args["node_3"] = {inputs, outputs};
  graph.AddNode("node_3", "Add_Fake", "node 3", inputs, outputs);

  inputs.clear();
  inputs.push_back(output_arg3);
  NodeArg *output_arg4 = new NodeArg("node_4_out_1", &tensor_int32);
  outputs.clear();
  outputs.push_back(output_arg4);
  expected_node_name_to_input_output_args["node_4"] = {inputs, outputs};
  graph.AddNode("node_4", "NoOp_Fake", "node 4", inputs, outputs);
  auto status = graph.Resolve();
  EXPECT_TRUE(status.IsOK());

  EXPECT_TRUE(Model::Save(model, "graph_1.pb").IsOK());
  std::shared_ptr<Model> model2;
  EXPECT_TRUE(Model::Load("graph_1.pb", &model2).IsOK());

  auto &model_proto = model.ToProto();
  auto &model_proto2 = model2->ToProto();
  bool equal_proto_1_and_2 = MessageDifferencer::MessageDifferencer::Equals(model_proto, model_proto2);
  std::string diff;
  if (!equal_proto_1_and_2) {
    MessageDifferencer d;
    d.ReportDifferencesToString(&diff);
    d.Compare(model_proto, model_proto2);
  } else {
    diff = "it's fine";
  }
  EXPECT_TRUE(equal_proto_1_and_2) << diff;

  // Load the model again to ensure that it's still the right thing.
  EXPECT_EQ(Model::Load(model_proto2, &model2), Status::OK());
  Graph *graph2 = model2->MainGraph();
  for (auto &node : graph2->Nodes()) {
    if (graph2->IsSourceNode(node.Index()) || graph2->IsSinkNode(node.Index())) {
      continue;
    }
    auto node_name_to_input_output_iter = expected_node_name_to_input_output_args.find(node.Name());
    EXPECT_FALSE(node_name_to_input_output_iter == expected_node_name_to_input_output_args.end());

    EXPECT_EQ(node_name_to_input_output_iter->second.first.size(), node.InputDefs().size());
    for (size_t i = 0; i < node_name_to_input_output_iter->second.first.size(); ++i) {
      EXPECT_EQ(node_name_to_input_output_iter->second.first[i]->Name(), node.InputDefs()[i]->Name());
      EXPECT_EQ(node_name_to_input_output_iter->second.first[i]->Type(), node.InputDefs()[i]->Type());
    }

    EXPECT_EQ(node_name_to_input_output_iter->second.second.size(), node.OutputDefs().size());
    for (size_t i = 0; i < node_name_to_input_output_iter->second.second.size(); ++i) {
      EXPECT_EQ(node_name_to_input_output_iter->second.second[i]->Name(), node.OutputDefs()[i]->Name());
      EXPECT_EQ(node_name_to_input_output_iter->second.second[i]->Type(), node.OutputDefs()[i]->Type());
    }
  }

  // Case 2 : The graph is not acyclic.  node_1 -> node_3 -> node_4 -> node_1.
  NodeTestHelper::MutableDefinitions(*node_1).input_defs[0] = output_arg4;
  status = graph.Resolve();
  EXPECT_FALSE(status.IsOK());
  EXPECT_EQ("Error: the graph is not acyclic.", status.ErrorMessage());

  LotusIR::Model model_2("graph_1");
  auto &graph_2 = *(model_2.MainGraph());

  onnx::TensorProto weight;
  weight.add_dims(1);
  weight.set_data_type(TensorProto_DataType_STRING);
  weight.add_string_data("test");
  weight.set_name("node_1_in_2");
  graph_2.AddInitializedTensor(weight);

  auto status_2 = graph_2.Resolve();
  EXPECT_TRUE(status_2.IsOK());

  //delete input_arg;
  //delete output_arg;
  //delete input_arg2;
  //delete output_arg2;
  //delete output_arg3;
  //delete output_arg4;
}

TEST(ResolvingGraphTest, GraphConstruction_TypeInference) {
  ONNX_OPERATOR_SCHEMA(Variable2_Fake)
      .SetDoc("Input variable.")
      .Input(0, "input_1", "docstr for input_1.", "T")
      .Output(0, "output_1", "docstr for output_1.", "T")
      .TypeConstraint("T", {"tensor(int32)", "tensor(float)"}, "input/output types");

  ONNX_OPERATOR_SCHEMA(Max_Fake)
      .SetDoc("Add two integers.")
      .Input(0, "input_1", "docstr for input_1.", "T")
      .Input(1, "input_2", "docstr for input_2.", "T")
      .Input(2, "input_3", "docstr for input_3.", "T")
      .Output(0, "output_1", "docstr for output_1.", "T")
      .TypeConstraint("T", {"tensor(int32)", "tensor(float)"}, "input/output types");

  Model model("graph_1");
  auto graph = model.MainGraph();

  // Case 1: A normal graph.
  //                         SourceNode
  //                   /         |         \
  //  node_1 (Variable)  node_2 (Variable)  node_3 (Variable)
  //                   \         |         / (it's all 3 nodes above outputs to the one input of node_4)
  //                        node_4 (Max)
  //                             |
  //                          SinkNode
  std::vector<NodeArg *> inputs;
  std::vector<NodeArg *> outputs;

  TypeProto tensor_int32;
  tensor_int32.mutable_tensor_type()->set_elem_type(TensorProto_DataType_INT32);
  tensor_int32.mutable_tensor_type()->mutable_shape()->add_dim()->set_dim_value(1);

  NodeArg *input_arg = new NodeArg("node_1_in_1", &tensor_int32);
  inputs.push_back(input_arg);
  NodeArg *output_arg = new NodeArg("node_1_out_1", &tensor_int32);
  outputs.push_back(output_arg);
  graph->AddNode("node_1", "Variable2_Fake", "node 1", inputs, outputs);

  inputs.clear();
  inputs.push_back(input_arg);
  NodeArg *output_arg2 = new NodeArg("node_2_out_1", &tensor_int32);
  outputs.clear();
  outputs.push_back(output_arg2);
  auto node_2 = graph->AddNode("node_2", "Variable2_Fake", "node 2", inputs, outputs);

  NodeArg *input_arg3 = new NodeArg("node_3_in_1", &tensor_int32);
  inputs.clear();
  inputs.push_back(input_arg3);
  NodeArg *output_arg3 = new NodeArg("node_3_out_1", &tensor_int32);
  outputs.clear();
  outputs.push_back(output_arg3);
  graph->AddNode("node_3", "Variable2_Fake", "node 3", inputs, outputs);

  inputs.clear();
  inputs.push_back(output_arg);
  inputs.push_back(output_arg2);
  inputs.push_back(output_arg3);
  NodeArg *output_arg4 = new NodeArg("node_4_out_1", &tensor_int32);
  outputs.clear();
  outputs.push_back(output_arg4);
  auto node_4 = graph->AddNode("node_4", "Max_Fake", "node 4", inputs, outputs);
  EXPECT_NE(node_4, nullptr);
  auto status = graph->Resolve();
  EXPECT_TRUE(status.IsOK());

  std::unordered_set<std::string> expected_graph_inputs = {"node_1_in_1", "node_3_in_1"};
  EXPECT_EQ(2, graph->GetInputs().size());
  for (auto &graph_input : graph->GetInputs()) {
    EXPECT_TRUE(expected_graph_inputs.find(graph_input->Name()) != expected_graph_inputs.end());
  }
  EXPECT_EQ(1, graph->GetOutputs().size());
  EXPECT_EQ("node_4_out_1", graph->GetOutputs()[0]->Name());
  EXPECT_EQ(2, graph->GetInputs().size());

  EXPECT_TRUE(Model::Save(model, "model_x.pb").IsOK());
  std::shared_ptr<Model> loaded_model;
  EXPECT_TRUE(Model::Load("model_x.pb", &loaded_model).IsOK());
  EXPECT_EQ(2, loaded_model->MainGraph()->GetInputs().size());

  auto &graph_proto = graph->ToGraphProto();
  EXPECT_EQ(2, graph_proto.input_size());
  for (auto &graphProtoInput : graph_proto.input()) {
    EXPECT_TRUE(expected_graph_inputs.find(graphProtoInput.name()) != expected_graph_inputs.end());
  }
  EXPECT_EQ(1, graph_proto.output_size());
  EXPECT_EQ("node_4_out_1", graph_proto.output(0).name());

  TypeProto tensor_float;
  tensor_float.mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT);
  NodeTestHelper::MutableDefinitions(*node_2).input_defs[0]->SetType(tensor_float);
  NodeTestHelper::MutableDefinitions(*node_2).output_defs[0]->SetType(tensor_float);
  status = graph->Resolve();
  EXPECT_FALSE(status.IsOK());
  EXPECT_EQ("Node (node_4) has different input types (tensor(float),tensor(int32)) matching to same type string (T).", status.ErrorMessage());

  //delete input_arg;
  //delete output_arg;
  //delete output_arg2;
  //delete input_arg3;
  //delete output_arg3;
  //delete output_arg4;
}

TEST(TestAddAttribute, AddTensorAttribute) {
  ONNX_OPERATOR_SCHEMA(__Constant)
      .SetDoc("Constant Op.")
      .Attr(kConstantValue, "constant value", AttrType::AttributeProto_AttributeType_TENSOR)
      .Output(0, "output_1", "docstr for output_1.", "tensor(int64)");
  std::vector<NodeArg *> inputs;
  std::vector<NodeArg *> outputs;
  Model model("graph_1");
  auto graph = model.MainGraph();
  TypeProto output_type;
  output_type.mutable_tensor_type()->set_elem_type(TensorProto_DataType_INT64);
  TensorShapeProto output_shape;
  output_shape.mutable_dim()->Add()->set_dim_value(1);
  output_shape.mutable_dim()->Add()->set_dim_value(3);
  *(output_type.mutable_tensor_type()->mutable_shape()) = output_shape;
  NodeArg *output_arg = new NodeArg("node_1_out_1", &output_type);
  outputs.push_back(output_arg);
  auto node_1 = graph->AddNode("node_1", "__Constant", "node 1.", inputs, outputs);
  TensorProto t;
  t.set_data_type(TensorProto_DataType_INT64);
  *(t.mutable_int64_data()->Add()) = 1;
  *(t.mutable_int64_data()->Add()) = 2;
  *(t.mutable_int64_data()->Add()) = 3;
  *(t.mutable_dims()->Add()) = 1;
  *(t.mutable_dims()->Add()) = 3;
  node_1->AddAttribute(kConstantValue, t);
  auto status = graph->Resolve();
  EXPECT_TRUE(status.IsOK());
  //delete output_arg;
}

}  // namespace Test
}  // namespace LotusIR
