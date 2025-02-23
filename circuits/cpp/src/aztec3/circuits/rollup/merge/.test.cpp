#include "c_bind.h"
#include "index.hpp"
#include "init.hpp"

#include "aztec3/circuits/rollup/merge/init.hpp"
#include "aztec3/circuits/rollup/test_utils/utils.hpp"

#include <gtest/gtest.h>

#include <gtest/gtest-death-test.h>

namespace {
using aztec3::circuits::rollup::merge::MergeRollupInputs;
using DummyComposer = aztec3::utils::DummyComposer;

using aztec3::circuits::rollup::test_utils::utils::get_empty_kernel;
using aztec3::circuits::rollup::test_utils::utils::get_merge_rollup_inputs;

using NT = aztec3::utils::types::NativeTypes;

using KernelData = aztec3::circuits::abis::PreviousKernelData<NT>;

}  // namespace
namespace aztec3::circuits::rollup::merge::native_merge_rollup_circuit {

class merge_rollup_tests : public ::testing::Test {
  protected:
    static void run_cbind(MergeRollupInputs& merge_rollup_inputs,
                          BaseOrMergeRollupPublicInputs& expected_public_inputs,
                          bool compare_pubins = true)
    {
        info("Retesting via cbinds....");
        std::vector<uint8_t> merge_rollup_inputs_vec;
        write(merge_rollup_inputs_vec, merge_rollup_inputs);

        uint8_t const* public_inputs_buf = nullptr;
        // info("simulating circuit via cbind");
        size_t public_inputs_size = 0;
        info("creating proof");
        auto* circuit_failure_ptr =
            merge_rollup__sim(merge_rollup_inputs_vec.data(), &public_inputs_size, &public_inputs_buf);
        ASSERT_TRUE(circuit_failure_ptr == nullptr);
        // info("PublicInputs size: ", public_inputs_size);

        if (compare_pubins) {
            BaseOrMergeRollupPublicInputs public_inputs;
            uint8_t const* public_inputs_buf_tmp = public_inputs_buf;
            read(public_inputs_buf_tmp, public_inputs);
            ASSERT_EQ(public_inputs.calldata_hash.size(), expected_public_inputs.calldata_hash.size());
            for (size_t i = 0; i < public_inputs.calldata_hash.size(); i++) {
                ASSERT_EQ(public_inputs.calldata_hash[i], expected_public_inputs.calldata_hash[i]);
            }

            std::vector<uint8_t> expected_public_inputs_vec;
            write(expected_public_inputs_vec, expected_public_inputs);

            ASSERT_EQ(public_inputs_size, expected_public_inputs_vec.size());
            // Just compare the first 10 bytes of the serialized public outputs
            if (public_inputs_size > 10) {
                // for (size_t 0; i < public_inputs_size; i++) {
                for (size_t i = 0; i < 10; i++) {
                    ASSERT_EQ(public_inputs_buf[i], expected_public_inputs_vec[i]);
                }
            }
        }
        free((void*)public_inputs_buf);
    }
};

TEST_F(merge_rollup_tests, native_different_rollup_type_fails)
{
    DummyComposer composer = DummyComposer("merge_rollup_tests__native_different_rollup_type_fails");
    std::array<KernelData, 4> const kernels = {
        get_empty_kernel(), get_empty_kernel(), get_empty_kernel(), get_empty_kernel()
    };
    MergeRollupInputs mergeInput = get_merge_rollup_inputs(composer, kernels);
    mergeInput.previous_rollup_data[0].base_or_merge_rollup_public_inputs.rollup_type = 0;
    mergeInput.previous_rollup_data[1].base_or_merge_rollup_public_inputs.rollup_type = 1;
    merge_rollup_circuit(composer, mergeInput);
    ASSERT_TRUE(composer.failed());
    ASSERT_EQ(composer.get_first_failure().message, "input proofs are of different rollup types");
}

TEST_F(merge_rollup_tests, native_different_rollup_height_fails)
{
    DummyComposer composer = DummyComposer("merge_rollup_tests__native_different_rollup_height_fails");
    std::array<KernelData, 4> const kernels = {
        get_empty_kernel(), get_empty_kernel(), get_empty_kernel(), get_empty_kernel()
    };
    MergeRollupInputs mergeInput = get_merge_rollup_inputs(composer, kernels);
    mergeInput.previous_rollup_data[0].base_or_merge_rollup_public_inputs.rollup_subtree_height = 0;
    mergeInput.previous_rollup_data[1].base_or_merge_rollup_public_inputs.rollup_subtree_height = 1;
    merge_rollup_circuit(composer, mergeInput);
    ASSERT_TRUE(composer.failed());
    ASSERT_EQ(composer.get_first_failure().message, "input proofs are of different rollup heights");
}

TEST_F(merge_rollup_tests, native_constants_different_failure)
{
    DummyComposer composer = DummyComposer("merge_rollup_tests__native_constants_different_failure");
    std::array<KernelData, 4> const kernels = {
        get_empty_kernel(), get_empty_kernel(), get_empty_kernel(), get_empty_kernel()
    };
    MergeRollupInputs inputs = get_merge_rollup_inputs(composer, kernels);
    inputs.previous_rollup_data[0].base_or_merge_rollup_public_inputs.constants.public_kernel_vk_tree_root = fr(1);
    inputs.previous_rollup_data[1].base_or_merge_rollup_public_inputs.constants.public_kernel_vk_tree_root = fr(0);
    merge_rollup_circuit(composer, inputs);
    ASSERT_TRUE(composer.failed());
    ASSERT_EQ(composer.get_first_failure().message, "input proofs have different constants");
}

TEST_F(merge_rollup_tests, native_fail_if_previous_rollups_dont_follow_on)
{
    DummyComposer composerA = DummyComposer("merge_rollup_tests__native_fail_if_previous_rollups_dont_follow_on_A");
    std::array<KernelData, 4> const kernels = {
        get_empty_kernel(), get_empty_kernel(), get_empty_kernel(), get_empty_kernel()
    };
    MergeRollupInputs const inputs = get_merge_rollup_inputs(composerA, kernels);
    auto inputA = inputs;
    inputA.previous_rollup_data[0].base_or_merge_rollup_public_inputs.end_private_data_tree_snapshot = {
        .root = fr(0), .next_available_leaf_index = 0
    };
    inputA.previous_rollup_data[1].base_or_merge_rollup_public_inputs.start_private_data_tree_snapshot = {
        .root = fr(1), .next_available_leaf_index = 0
    };

    merge_rollup_circuit(composerA, inputA);
    ASSERT_TRUE(composerA.failed());
    ASSERT_EQ(composerA.get_first_failure().message, "input proofs have different private data tree snapshots");

    // do the same for nullifier tree
    DummyComposer composerB = DummyComposer("merge_rollup_tests__native_fail_if_previous_rollups_dont_follow_on_B");
    auto inputB = inputs;

    inputB.previous_rollup_data[0].base_or_merge_rollup_public_inputs.end_nullifier_tree_snapshot = {
        .root = fr(0), .next_available_leaf_index = 0
    };
    inputB.previous_rollup_data[1].base_or_merge_rollup_public_inputs.start_nullifier_tree_snapshot = {
        .root = fr(1), .next_available_leaf_index = 0
    };
    merge_rollup_circuit(composerB, inputB);
    ASSERT_TRUE(composerB.failed());
    ASSERT_EQ(composerB.get_first_failure().message, "input proofs have different nullifier tree snapshots");

    // do the same for contract tree
    DummyComposer composerC = DummyComposer("merge_rollup_tests__native_fail_if_previous_rollups_dont_follow_on_C");
    auto inputC = inputs;
    inputC.previous_rollup_data[0].base_or_merge_rollup_public_inputs.end_contract_tree_snapshot = {
        .root = fr(0), .next_available_leaf_index = 0
    };
    inputC.previous_rollup_data[1].base_or_merge_rollup_public_inputs.start_contract_tree_snapshot = {
        .root = fr(1), .next_available_leaf_index = 0
    };
    merge_rollup_circuit(composerC, inputC);
    ASSERT_TRUE(composerC.failed());
    ASSERT_EQ(composerC.get_first_failure().message, "input proofs have different contract tree snapshots");
}

TEST_F(merge_rollup_tests, native_rollup_fields_are_set_correctly)
{
    DummyComposer composer = DummyComposer("merge_rollup_tests__native_rollup_fields_are_set_correctly");
    std::array<KernelData, 4> const kernels = {
        get_empty_kernel(), get_empty_kernel(), get_empty_kernel(), get_empty_kernel()
    };
    MergeRollupInputs inputs = get_merge_rollup_inputs(composer, kernels);
    BaseOrMergeRollupPublicInputs outputs = merge_rollup_circuit(composer, inputs);
    // check that rollup type is set to merge
    ASSERT_EQ(outputs.rollup_type, 1);
    // check that rollup height is incremented
    ASSERT_EQ(outputs.rollup_subtree_height,
              inputs.previous_rollup_data[0].base_or_merge_rollup_public_inputs.rollup_subtree_height + 1);

    // set inputs to have a merge rollup type and set the rollup height and test again.
    inputs.previous_rollup_data[0].base_or_merge_rollup_public_inputs.rollup_type = 1;
    inputs.previous_rollup_data[0].base_or_merge_rollup_public_inputs.rollup_subtree_height = 1;

    inputs.previous_rollup_data[1].base_or_merge_rollup_public_inputs.rollup_type = 1;
    inputs.previous_rollup_data[1].base_or_merge_rollup_public_inputs.rollup_subtree_height = 1;

    outputs = merge_rollup_circuit(composer, inputs);
    ASSERT_EQ(outputs.rollup_type, 1);
    ASSERT_EQ(outputs.rollup_subtree_height, 2);
    ASSERT_FALSE(composer.failed());
}

TEST_F(merge_rollup_tests, native_start_and_end_snapshots)
{
    DummyComposer composer = DummyComposer("merge_rollup_tests__native_start_and_end_snapshots");
    std::array<KernelData, 4> const kernels = {
        get_empty_kernel(), get_empty_kernel(), get_empty_kernel(), get_empty_kernel()
    };
    MergeRollupInputs inputs = get_merge_rollup_inputs(composer, kernels);
    BaseOrMergeRollupPublicInputs const outputs = merge_rollup_circuit(composer, inputs);
    // check that start and end snapshots are set correctly
    ASSERT_EQ(outputs.start_private_data_tree_snapshot,
              inputs.previous_rollup_data[0].base_or_merge_rollup_public_inputs.start_private_data_tree_snapshot);
    ASSERT_EQ(outputs.end_private_data_tree_snapshot,
              inputs.previous_rollup_data[1].base_or_merge_rollup_public_inputs.end_private_data_tree_snapshot);

    ASSERT_EQ(outputs.start_nullifier_tree_snapshot,
              inputs.previous_rollup_data[0].base_or_merge_rollup_public_inputs.start_nullifier_tree_snapshot);
    ASSERT_EQ(outputs.end_nullifier_tree_snapshot,
              inputs.previous_rollup_data[1].base_or_merge_rollup_public_inputs.end_nullifier_tree_snapshot);

    ASSERT_EQ(outputs.start_contract_tree_snapshot,
              inputs.previous_rollup_data[0].base_or_merge_rollup_public_inputs.start_contract_tree_snapshot);
    ASSERT_EQ(outputs.end_contract_tree_snapshot,
              inputs.previous_rollup_data[1].base_or_merge_rollup_public_inputs.end_contract_tree_snapshot);

    ASSERT_EQ(outputs.start_public_data_tree_root,
              inputs.previous_rollup_data[0].base_or_merge_rollup_public_inputs.start_public_data_tree_root);
    ASSERT_EQ(outputs.end_public_data_tree_root,
              inputs.previous_rollup_data[1].base_or_merge_rollup_public_inputs.end_public_data_tree_root);

    ASSERT_FALSE(composer.failed());
}

TEST_F(merge_rollup_tests, native_calldata_hash)
{
    DummyComposer composer = DummyComposer("merge_rollup_tests__native_calldata_hash");
    std::vector<uint8_t> const zero_bytes_vec = test_utils::utils::get_empty_calldata_leaf();
    auto call_data_hash_inner = sha256::sha256(zero_bytes_vec);

    std::array<uint8_t, 64> hash_input;
    for (uint8_t i = 0; i < 32; ++i) {
        hash_input[i] = call_data_hash_inner[i];
        hash_input[32 + i] = call_data_hash_inner[i];
    }

    std::vector<uint8_t> const calldata_hash_input_bytes_vec(hash_input.begin(), hash_input.end());

    auto expected_calldata_hash = sha256::sha256(calldata_hash_input_bytes_vec);

    std::array<KernelData, 4> const kernels = {
        get_empty_kernel(), get_empty_kernel(), get_empty_kernel(), get_empty_kernel()
    };
    MergeRollupInputs const inputs = get_merge_rollup_inputs(composer, kernels);

    BaseOrMergeRollupPublicInputs const outputs = merge_rollup_circuit(composer, inputs);

    std::array<fr, 2> actual_calldata_hash_fr = outputs.calldata_hash;
    auto high_buffer = actual_calldata_hash_fr[0].to_buffer();
    auto low_buffer = actual_calldata_hash_fr[1].to_buffer();

    std::array<uint8_t, 32> actual_calldata_hash;
    for (uint8_t i = 0; i < 16; ++i) {
        actual_calldata_hash[i] = high_buffer[16 + i];
        actual_calldata_hash[16 + i] = low_buffer[16 + i];
    }

    ASSERT_EQ(expected_calldata_hash, actual_calldata_hash);
    ASSERT_FALSE(composer.failed());
}

TEST_F(merge_rollup_tests, native_constants_dont_change)
{
    DummyComposer composer = DummyComposer("merge_rollup_tests__native_constants_dont_change");
    std::array<KernelData, 4> const kernels = {
        get_empty_kernel(), get_empty_kernel(), get_empty_kernel(), get_empty_kernel()
    };
    MergeRollupInputs inputs = get_merge_rollup_inputs(composer, kernels);

    BaseOrMergeRollupPublicInputs const outputs = merge_rollup_circuit(composer, inputs);
    ASSERT_EQ(inputs.previous_rollup_data[0].base_or_merge_rollup_public_inputs.constants, outputs.constants);
    ASSERT_EQ(inputs.previous_rollup_data[1].base_or_merge_rollup_public_inputs.constants, outputs.constants);
}

TEST_F(merge_rollup_tests, native_aggregate)
{
    // TODO: Fix this when aggregation works
    DummyComposer composer = DummyComposer("merge_rollup_tests__native_aggregate");
    std::array<KernelData, 4> const kernels = {
        get_empty_kernel(), get_empty_kernel(), get_empty_kernel(), get_empty_kernel()
    };
    MergeRollupInputs inputs = get_merge_rollup_inputs(composer, kernels);

    BaseOrMergeRollupPublicInputs const outputs = merge_rollup_circuit(composer, inputs);
    ASSERT_EQ(inputs.previous_rollup_data[0].base_or_merge_rollup_public_inputs.end_aggregation_object.public_inputs,
              outputs.end_aggregation_object.public_inputs);
    ASSERT_FALSE(composer.failed());
}

TEST_F(merge_rollup_tests, native_merge_cbind)
{
    DummyComposer composer = DummyComposer("merge_rollup_tests__native_merge_cbind");
    std::array<KernelData, 4> const kernels = {
        get_empty_kernel(), get_empty_kernel(), get_empty_kernel(), get_empty_kernel()
    };
    MergeRollupInputs inputs = get_merge_rollup_inputs(composer, kernels);

    ASSERT_FALSE(composer.failed());
    BaseOrMergeRollupPublicInputs ignored_public_inputs;
    run_cbind(inputs, ignored_public_inputs, false);
}
}  // namespace aztec3::circuits::rollup::merge::native_merge_rollup_circuit
