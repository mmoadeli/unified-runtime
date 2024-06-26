// Copyright (C) 2023 Intel Corporation
// Part of the Unified-Runtime Project, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE.TXT
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <map>
#include <uur/fixtures.h>

std::unordered_map<ur_queue_info_t, size_t> queue_info_size_map = {
    {UR_QUEUE_INFO_CONTEXT, sizeof(ur_context_handle_t)},
    {UR_QUEUE_INFO_DEVICE, sizeof(ur_device_handle_t)},
    {UR_QUEUE_INFO_DEVICE_DEFAULT, sizeof(ur_queue_handle_t)},
    {UR_QUEUE_INFO_FLAGS, sizeof(ur_queue_flags_t)},
    {UR_QUEUE_INFO_REFERENCE_COUNT, sizeof(uint32_t)},
    {UR_QUEUE_INFO_SIZE, sizeof(uint32_t)},
    {UR_QUEUE_INFO_EMPTY, sizeof(ur_bool_t)},
};

using urQueueGetInfoTestWithInfoParam =
    uur::urQueueTestWithParam<ur_queue_info_t>;

UUR_TEST_SUITE_P(urQueueGetInfoTestWithInfoParam,
                 ::testing::Values(UR_QUEUE_INFO_CONTEXT, UR_QUEUE_INFO_DEVICE,
                                   UR_QUEUE_INFO_FLAGS,
                                   UR_QUEUE_INFO_REFERENCE_COUNT,
                                   UR_QUEUE_INFO_EMPTY),
                 uur::deviceTestWithParamPrinter<ur_queue_info_t>);

TEST_P(urQueueGetInfoTestWithInfoParam, Success) {
    ur_queue_info_t info_type = getParam();
    size_t size = 0;
    auto result = urQueueGetInfo(queue, info_type, 0, nullptr, &size);

    if (result == UR_RESULT_SUCCESS) {
        ASSERT_NE(size, 0);

        if (const auto expected_size = queue_info_size_map.find(info_type);
            expected_size != queue_info_size_map.end()) {
            ASSERT_EQ(expected_size->second, size);
        }

        std::vector<uint8_t> data(size);
        ASSERT_SUCCESS(
            urQueueGetInfo(queue, info_type, size, data.data(), nullptr));

        switch (info_type) {
        case UR_QUEUE_INFO_CONTEXT: {
            auto returned_context =
                reinterpret_cast<ur_context_handle_t *>(data.data());
            ASSERT_EQ(context, *returned_context);
            break;
        }
        case UR_QUEUE_INFO_DEVICE: {
            auto returned_device =
                reinterpret_cast<ur_device_handle_t *>(data.data());
            ASSERT_EQ(*returned_device, device);
            break;
        }
        case UR_QUEUE_INFO_REFERENCE_COUNT: {
            auto returned_reference_count =
                reinterpret_cast<uint32_t *>(data.data());
            ASSERT_GT(*returned_reference_count, 0U);
            break;
        }
        default:
            break;
        }
    } else {
        ASSERT_EQ_RESULT(result, UR_RESULT_ERROR_UNSUPPORTED_ENUMERATION);
    }
}

struct urQueueGetInfoDeviceQueueTestWithInfoParam
    : public uur::urContextTestWithParam<ur_queue_info_t> {
    void SetUp() {
        urContextTestWithParam<ur_queue_info_t>::SetUp();
        ur_queue_flags_t deviceQueueCapabilities;
        ASSERT_SUCCESS(
            urDeviceGetInfo(device, UR_DEVICE_INFO_QUEUE_ON_DEVICE_PROPERTIES,
                            sizeof(deviceQueueCapabilities),
                            &deviceQueueCapabilities, nullptr));
        if (!deviceQueueCapabilities) {
            GTEST_SKIP() << "Queue on device is not supported.";
        }
        ASSERT_SUCCESS(
            urQueueCreate(context, device, &queueProperties, &queue));
    }

    void TearDown() {
        if (queue) {
            ASSERT_SUCCESS(urQueueRelease(queue));
        }
        urContextTestWithParam<ur_queue_info_t>::TearDown();
    }

    ur_queue_handle_t queue = nullptr;
    ur_queue_properties_t queueProperties = {
        UR_STRUCTURE_TYPE_QUEUE_PROPERTIES, nullptr,
        UR_QUEUE_FLAG_ON_DEVICE | UR_QUEUE_FLAG_ON_DEVICE_DEFAULT |
            UR_QUEUE_FLAG_OUT_OF_ORDER_EXEC_MODE_ENABLE};
};

UUR_TEST_SUITE_P(urQueueGetInfoDeviceQueueTestWithInfoParam,
                 ::testing::Values(UR_QUEUE_INFO_CONTEXT, UR_QUEUE_INFO_DEVICE,
                                   UR_QUEUE_INFO_DEVICE_DEFAULT,
                                   UR_QUEUE_INFO_FLAGS,
                                   UR_QUEUE_INFO_REFERENCE_COUNT,
                                   UR_QUEUE_INFO_SIZE, UR_QUEUE_INFO_EMPTY),
                 uur::deviceTestWithParamPrinter<ur_queue_info_t>);

TEST_P(urQueueGetInfoDeviceQueueTestWithInfoParam, Success) {
    ur_queue_info_t info_type = getParam();
    size_t size = 0;
    auto result = urQueueGetInfo(queue, info_type, 0, nullptr, &size);

    if (result == UR_RESULT_SUCCESS) {
        ASSERT_NE(size, 0);

        if (const auto expected_size = queue_info_size_map.find(info_type);
            expected_size != queue_info_size_map.end()) {
            ASSERT_EQ(expected_size->second, size);
        }

        std::vector<uint8_t> data(size);
        ASSERT_SUCCESS(
            urQueueGetInfo(queue, info_type, size, data.data(), nullptr));
    } else {
        ASSERT_EQ_RESULT(result, UR_RESULT_ERROR_UNSUPPORTED_ENUMERATION);
    }
}

using urQueueGetInfoTest = uur::urQueueTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urQueueGetInfoTest);

TEST_P(urQueueGetInfoTest, InvalidNullHandleQueue) {
    ur_context_handle_t context = nullptr;
    ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                     urQueueGetInfo(nullptr, UR_QUEUE_INFO_CONTEXT,
                                    sizeof(ur_context_handle_t), &context,
                                    nullptr));
}

TEST_P(urQueueGetInfoTest, InvalidEnumerationProperty) {
    ur_context_handle_t context = nullptr;
    ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_ENUMERATION,
                     urQueueGetInfo(queue, UR_QUEUE_INFO_FORCE_UINT32,
                                    sizeof(ur_context_handle_t), &context,
                                    nullptr));
}

TEST_P(urQueueGetInfoTest, InvalidSizeZero) {
    ur_context_handle_t context = nullptr;
    ASSERT_EQ_RESULT(
        UR_RESULT_ERROR_INVALID_SIZE,
        urQueueGetInfo(queue, UR_QUEUE_INFO_CONTEXT, 0, &context, nullptr));
}

TEST_P(urQueueGetInfoTest, InvalidSizeSmall) {
    ur_context_handle_t context = nullptr;
    ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_SIZE,
                     urQueueGetInfo(queue, UR_QUEUE_INFO_CONTEXT,
                                    sizeof(ur_context_handle_t) - 1, &context,
                                    nullptr));
}

TEST_P(urQueueGetInfoTest, InvalidNullPointerPropValue) {
    ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                     urQueueGetInfo(queue, UR_QUEUE_INFO_CONTEXT,
                                    sizeof(ur_context_handle_t), nullptr,
                                    nullptr));
}

TEST_P(urQueueGetInfoTest, InvalidNullPointerPropSizeRet) {
    ASSERT_EQ_RESULT(
        UR_RESULT_ERROR_INVALID_NULL_POINTER,
        urQueueGetInfo(queue, UR_QUEUE_INFO_CONTEXT, 0, nullptr, nullptr));
}
