// uvc.state (rules 31 and 64). Runs on TestBench's listener thread; every request is answered on the game thread
// (Pump) and waited for.
#include "Tool.h"

#include "Settings.h"
#include "TestBenchAPI.h"
#include "Tick.h"
#include "Unbind.h"

#include <condition_variable>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tool
{
	namespace
	{
		TestBenchAPI::ITestBenchInterface001* g_tb = nullptr;
		std::mutex              g_lock;
		std::condition_variable g_cv;
		bool                    g_wanted = false, g_done = false;
		json                    g_request, g_answer;

		void Write(void* a_sink, TestBenchAPI::WriteFn a_write, const json& a_j) { a_write(a_sink, a_j.dump().c_str()); }

		json Answer(const json& a_req)
		{
			const std::string op = a_req.value("op", "state");
			if (op == "rows") {
				return { { "rows", unbind::Rows() } };
			}
			if (op == "select") {
				return { { "result", unbind::Select(a_req.value("row", 0u), a_req.value("slot", std::string("primary")),
									 a_req.value("key", std::string())) } };
			}
			const auto s = unbind::GetStatus();
			return { { "version", UVC_VERSION }, { "ticks", tick::Reads() }, { "enabled", settings::Get().enabled },
				{ "watching_keyboard", s.watchingKeyboard }, { "watching_controller", s.watchingGamepad },
				{ "listens", s.listens }, { "selections", s.selections }, { "unbinds", s.unbinds },
				{ "problem", s.problem }, { "recent", s.recent } };
		}

		void Tool(void*, const char* a_args, void* a_sink, TestBenchAPI::WriteFn a_write)
		{
			json args = json::parse(a_args ? a_args : "{}", nullptr, false);
			if (args.is_discarded() || !args.is_object()) {
				args = json::object();
			}
			const std::string op = args.value("op", "state");
			if (op != "state" && op != "rows" && op != "select") {
				Write(a_sink, a_write, { { "ok", false }, { "error", "op: state | rows | select {row, slot: primary|secondary|gamepad, key}" } });
				return;
			}
			std::unique_lock l(g_lock);
			g_request = args;
			g_wanted = true;
			g_done = false;
			if (!g_cv.wait_for(l, 3s, [] { return g_done; })) {
				g_wanted = false;
				Write(a_sink, a_write, { { "ok", false }, { "error", "the game thread did not answer in 3 s" } });
				return;
			}
			json out = g_answer;
			out["ok"] = true;
			Write(a_sink, a_write, out);
		}
	}

	void Pump()
	{
		json req;
		{
			std::scoped_lock l(g_lock);
			if (!g_wanted) {
				return;
			}
			req = g_request;
		}
		json a = Answer(req);
		std::scoped_lock l(g_lock);
		g_answer = std::move(a);
		g_wanted = false;
		g_done = true;
		g_cv.notify_all();
	}

	bool Register()
	{
		if (g_tb) {
			return true;
		}
		HMODULE tb = ::GetModuleHandleW(L"TestBench.dll");
		auto get = tb ? reinterpret_cast<void* (*)(unsigned)>(::GetProcAddress(tb, "TestBench_GetInterface")) : nullptr;
		g_tb = get ? static_cast<TestBenchAPI::ITestBenchInterface001*>(get(1)) : nullptr;
		if (!g_tb) {
			return false;
		}
		g_tb->RegisterTool("uvc.state",
			R"({"description":"Unbind Vanilla Controls: op state (default) - watched row classes, counts, last events; op rows - every live Controls row with its keys; op select {row, slot: primary|secondary|gamepad, key} - drive a row's handler with a key as a press would","inputSchema":{"type":"object","properties":{"op":{"type":"string"},"row":{"type":"integer"},"slot":{"type":"string"},"key":{"type":"string"}}}})",
			&Tool, nullptr);
		logger::info("TestBench tool registered: uvc.state");
		return true;
	}
}
