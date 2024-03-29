#pragma once

#include "elevator/elevator_actor.hpp"
#include "elevator/elevator_fsm.hpp"

namespace elevator
{

	// Finite state machine for an elevator actor
	// See elevator_fsm.hpp for more details.

	// In general, specific states can elect to override on_exit(), on_enter, and the various handle_* event functions.
	// These functions then delegate to or coordinate actions on the associated actor.


	// Create the shared_ptrs for each of the elevator states, these are set as static variables
	// on class elevator_fsm. This pattern is OK for FSMs that don't need to store 'state' in states; if this is necessary
	// then the code just needs to be changed to create an instance of the template state (elevator_fsm in this instance) and separate instances
	// of the state classes for each actor.

	const std::shared_ptr<initialising_state> elevator_fsm::initalising = std::make_shared<initialising_state>();
	const std::shared_ptr<disconnected_state> elevator_fsm::disconnected = std::make_shared<disconnected_state>();
	const std::shared_ptr<idle_state> elevator_fsm::idle = std::make_shared<idle_state>();
	const std::shared_ptr<in_transit_state> elevator_fsm::in_transit = std::make_shared<in_transit_state>();
	const std::shared_ptr<at_waypoint_state> elevator_fsm::at_waypoint = std::make_shared<at_waypoint_state>();
	const std::shared_ptr<quitting_state> elevator_fsm::quitting = std::make_shared<quitting_state>();

	// Common quit action for all states..
	void elevator_fsm::handle_quit(elevator_actor& actor)
	{
		actor.transition_to_state(elevator_fsm::quitting);
	}

	// Common connect action for all states...assumes connection is dropped, reestablished, then back in lobby

	void elevator_fsm::handle_connect(elevator_actor& actor, std::string host, uint16_t port)
	{
		actor.on_connect(host, port); // NB: .connect is an async operation, and sets state itself.
	}


	/// Initialising

	void initialising_state::on_enter(elevator_actor& actor)
	{
		if (actor.on_initialise())
			actor.transition_to_state(elevator_fsm::disconnected);
		else
			actor.transition_to_state(elevator_fsm::quitting);
	}

	// common waypoint acceptance action for accepting states
	void waypoint_accepting_state::handle_waypoint_received(elevator_actor& actor, int waypoint_floor)
	{
		actor.on_waypoint_received(waypoint_floor);
		actor.fsm->handle_start(actor);
	}

	/// Idle

	void idle_state::on_enter(elevator_actor& actor)
	{
		actor.on_idle();
	}

	void idle_state::handle_start(elevator_actor& actor)
	{
		if (actor.on_start())
			actor.transition_to_state(elevator_fsm::in_transit);
	}

	// In Transit

	void in_transit_state::on_enter(elevator_actor& actor)
	{
		actor.on_in_transit();
		// start in transit timer cycle, in_transit will check 
		actor.timer_pulse(elevator::FLOOR_TRANSIT_TIME_SEC);
	}

	void in_transit_state::handle_timer(elevator_actor& actor)
	{
		// decrease/increase current floor, check against waypoint, transition to 
		// at_waypoint, or continue in_transit

		// for now just go to idle once waypoint is hit.
		if (actor.waypoint_floors.size() == 0)
		{
			actor.current_motion = elevator_motion::stationary;
			actor.transition_to_state(elevator_fsm::idle);
			return;
		}

		int next_waypoint_floor = actor.waypoint_floors.front();

		if (next_waypoint_floor > actor.current_floor)
			actor.current_motion = elevator_motion::moving_up;
		else if (next_waypoint_floor < actor.current_floor)
			actor.current_motion = elevator_motion::moving_down;
		else
			actor.current_motion = elevator_motion::stationary;

		switch (actor.current_motion)
		{
		case elevator_motion::stationary:
			// arrived at waypoint
			actor.debug_msg("stopping at waypoint floor: " + std::to_string(actor.current_floor));
			actor.waypoint_floors.pop();
			actor.transition_to_state(elevator_fsm::at_waypoint);
			break;
		case elevator_motion::moving_up:
			actor.debug_msg("passing floor: " + std::to_string(actor.current_floor));
			actor.current_floor++;
			actor.timer_pulse(elevator::FLOOR_TRANSIT_TIME_SEC);
			break;
		case elevator_motion::moving_down:
			actor.debug_msg("passing floor: " + std::to_string(actor.current_floor));
			actor.current_floor--;
			actor.timer_pulse(elevator::FLOOR_TRANSIT_TIME_SEC);
			break;
		default:
			break;
		}
	}

	/// At Waypoint (i.e. more waypoints to go, not idle)

	void at_waypoint_state::on_enter(elevator_actor& actor)
	{
		actor.on_waypoint_arrive();
		actor.timer_pulse(elevator::AT_WAYPOINT_TIME_SEC);
	}

	void at_waypoint_state::handle_timer(elevator_actor& actor)
	{
		if (actor.waypoint_floors.size() == 0)
		{
			actor.current_motion = elevator_motion::stationary;
			actor.debug_msg("no more waypoints, staying idle at floor: " + std::to_string(actor.current_floor));
			actor.transition_to_state(elevator_fsm::idle);
			return;
		} else {
			actor.debug_msg("more waypoints to go, doors closing!");
			actor.transition_to_state(elevator_fsm::in_transit);
		}
	}

	/// Quitting

	void quitting_state::on_enter(elevator_actor& actor)
	{
		actor.quit();
	}

}
