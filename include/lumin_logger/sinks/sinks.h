#pragma once

/**
 * @file sinks.h
 * @brief Common sink implementations for the Lumin logger system
 * 
 * This file includes all available custom sink implementations
 * for the Lumin logger system.
 */

#include "memory_sink.h"
#include "stats_sink.h"
#include "imgui_sink.h"

namespace lumin {

/**
 * @namespace sinks
 * @brief Contains all common sink implementations
 * 
 * This namespace contains all the pre-implemented custom sinks
 * that can be used with the Lumin logger.
 */
namespace sinks {

// Using declarations for easy access to factory functions
using lumin::create_memory_sink;
using lumin::create_stats_sink;
using lumin::create_imgui_sink;

// Using declarations for sink types
using MemorySink = lumin::MemorySink;
using StatsSink = lumin::StatsSink;
using ImGuiLogSink = lumin::ImGuiLogSink;

} // namespace sinks

} // namespace lumin 