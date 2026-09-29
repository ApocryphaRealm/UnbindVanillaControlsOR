#pragma once

// The TestBench driving tool (rules 31 and 64): uvc.state - op state (default): what is watched, counts and the last
// events; op rows: every live Controls row with its selectors' keys; op select {row, slot, key}: drive one row's
// handler with a key exactly as a press would (the Controls page must be open). Game thread via Pump.
namespace tool
{
	bool Register();   // once TestBench.dll is loaded (idempotent)
	void Pump();       // every tick, on the game thread: answers a pending request
}
