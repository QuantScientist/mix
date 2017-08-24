#pragma once
#include <mix/computer_listener.h>
#include <mix/io_device.h>
#include <mix/command.h>
#include <mix/word.h>

#include <gmock/gmock.h>

struct ComputerListenerMock :
	public mix::IComputerListener
{
	MOCK_METHOD1(on_memory_set, void (int));
	MOCK_METHOD0(on_ra_set, void ());
	MOCK_METHOD0(on_rx_set, void ());
	MOCK_METHOD1(on_ri_set, void (std::size_t));
	MOCK_METHOD0(on_overflow_flag_set, void ());
	MOCK_METHOD0(on_comparison_state_set, void ());
	MOCK_METHOD1(on_current_address_changed, void (int));
	MOCK_METHOD1(on_jump, void (int));
	MOCK_METHOD1(on_before_command, void (const mix::Command&));
	MOCK_METHOD1(on_after_command, void (const mix::Command&));
	MOCK_METHOD2(on_device_read, void (mix::DeviceId, mix::DeviceBlockId));
	MOCK_METHOD2(on_device_write, void (mix::DeviceId, mix::DeviceBlockId));
	MOCK_METHOD1(on_wait_on_device, void (mix::DeviceId));
};

struct DeviceMock :
	public mix::IIODevice
{
	MOCK_CONST_METHOD0(ready, bool());
	MOCK_CONST_METHOD0(block_size, int());
	MOCK_METHOD1(read_next, mix::Word (mix::DeviceBlockId));
	MOCK_METHOD2(write_next, void (mix::DeviceBlockId, const mix::Word&));
};



