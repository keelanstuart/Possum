// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt

#pragma once


namespace nucleus
{

	using RetCode = enum
	{
		OK = 0,

		UNKNOWN_ERROR,
		INVALID_PARAM,

		NULLFILESPEC,

		NOTFOUND,

		PLUG_NOACTIVATE,
		PLUG_NODEACTIVATE,

		DEVICE_EXISTS,
		DEVICE_NOEXIST,

		TRACK_NORETURNABLE,
		TRACK_UNKNOWNCOMMAND,
		TRACK_DUPLICATENAME,
		TRACK_NONAME,
		TRACK_INSUFFICIENTTIME,
		TRACK_TIMEOVERLAP,

		COMMAND_GIVEN_TO_WRONG_DEVICE,
		COMMAND_PAUSES,
		COMMAND_LOADS_SUBSHOW,
		COMMAND_JUMPS
	};

};