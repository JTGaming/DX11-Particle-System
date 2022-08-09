#pragma once
#include <chrono>
#include <unordered_map>

#include "Variables.h"

#define SCOPE_EXEC_TIME(x) const AutoProfiler ap(x)

struct profile
{
	std::chrono::duration<float, std::milli> ft_aggr{};
	uint32_t count{};
	std::chrono::steady_clock::time_point start_time{};
};

class Profiler
{
public:
	Profiler()
	{
		auto time1 = std::chrono::high_resolution_clock::now();
		auto time2 = std::chrono::high_resolution_clock::now(); //-V656
		time_wasted_per_call = time2 - time1;
	}

	void start(const std::string& name)
	{
		auto& map_item = calls[name];
		map_item.start_time = std::chrono::high_resolution_clock::now();
	}

	void end(const std::string& name)
	{
		auto time = std::chrono::high_resolution_clock::now();
		auto& map_item = calls[name];

		map_item.ft_aggr += time - map_item.start_time - time_wasted_per_call;
		map_item.count++;
	}

	static constexpr const char* string_names[] =
	{
		"B",
		"KB",
		"MB",
		"GB"
	};

	std::string convert_bytes(float bytes)
	{
		char head[32];

		int32_t selector = 0;
		while (bytes > 500.f)
		{
			bytes *= 0.001f;
			selector++;
		}
		sprintf_s(head, "Throughput: %.2f %s/sec", bytes, string_names[selector]);
		return head;
	}

	void calc_avg()
	{
		auto time = std::chrono::high_resolution_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time - last_update_time).count();
		if (ms > UPDATE_FREQ)
		{
			//std::string format;


			avg_timeframes.clear();
			g_Options.Menu.AvgThroughput = convert_bytes(g_Options.Menu.Throughput * 1000.f / ms);
			g_Options.Menu.Throughput = 0;
			const auto frame_count = calls["main_loop"].count;
			for (auto& map : calls)
			{
				if (map.second.count)
					avg_timeframes.emplace_back(map.first, map.second.ft_aggr.count() / frame_count);

				//format += map.first + ": " + std::to_string(avg * 100) + " %%\n";
				map.second = profile();
			}
			std::sort(avg_timeframes.begin(), avg_timeframes.end(), [](const std::pair<std::string, float>& item1, const std::pair<std::string, float>& item2) { return item1.second > item2.second; });

			//format += '\n';
			//OutputDebugStringA(format.c_str());
			last_update_time = time;
		}
	}

	std::vector<std::pair<std::string, float>> avg_timeframes{};

private:
	std::unordered_map<std::string, profile> calls{};

	static constexpr uint32_t UPDATE_FREQ = 1000;
	std::chrono::steady_clock::time_point last_update_time{};
	std::chrono::nanoseconds time_wasted_per_call{};
};
extern Profiler g_Profiler;

class AutoProfiler
{
public:
	AutoProfiler(const std::string& c) : caller(c)
	{
		g_Profiler.start(caller);
	}
	~AutoProfiler()
	{
		g_Profiler.end(caller);
	}

private:
	const std::string caller;
};
