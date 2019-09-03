#include <iostream>
#include <vector>
#include <string>

#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "string_util.hpp"

#include "elevator.hpp"

#include "elevator_actor.hpp"
#include "elevator_fsm.hpp"

#include <cassert>

using namespace caf;
using namespace std;
using namespace elevator;


namespace elevator
{
	// override for actor behaviour
	// note that messages received by the actor are immediately delegated to the FSM
	behavior elevator_actor::make_behavior()
	{
		return {
			[=](elevator::quit_atom)
			{
				debug_msg("quit_atom received");
				fsm->handle_quit(*this);
			},
			[=](connect_to_controller_atom, const std::string& host, uint16_t port)
			{
				debug_msg("connect_to_controller_atom received, host: " + host + ", port: " + std::to_string(port));
				this->controller_host = host;
				this->controller_port = port;
				fsm->handle_connect(*this, host, port);
			},
			[=](elevator::waypoint_atom, int waypoint_floor)
			{
				debug_msg("waypoint_atom received, for floor: " + std::to_string(waypoint_floor));
				fsm->handle_waypoint_received(*this, waypoint_floor);
			},
			[=](get_current_floor_atom)
			{
				//aout(this) << "\nelevator: get_current_floor_atom received" << endl;
				return current_floor;
			},
			[=](get_current_state_name_atom)
			{
				//aout(this) << "\nelevator: get_current_state_name_atom received" << endl;
				return fsm->get_state_name();
			},
			[=](timer_atom)
			{
				debug_msg("timer_atom received");
				return fsm->handle_timer(*this);
			}
		};
	}

	// set next state, calling on_exit and on_enter functions
	void elevator_actor::transition_to_state(std::shared_ptr<elevator_fsm> state)
	{
		if (this->fsm)
			this->fsm->on_exit(*this);
		this->fsm = state;
		this->fsm->on_enter(*this);
	}

	// cya
	void elevator_actor::on_quit()
	{

		anon_send_exit(this, exit_reason::user_shutdown);
	}

	// set up
	bool elevator_actor::on_initialise()
	{
		// transition to `unconnected` on elevator controller failure/shutdown
		// set the handler if we lose connection to elevator controller 
		set_down_handler([=](const down_msg& dm)
			{
				if (dm.source == controller)
				{
					debug_msg("lost connection to elevator controller, please reconnect or quit");
					this->controller = nullptr;
				}
			});

		return true;
	}

	// connect to the elevator controller
	void elevator_actor::on_connect(const std::string& host, uint16_t port)
	{
		// make sure we are not pointing to an old controller
		controller = nullptr;
		controller_host.assign("");
		controller_port = 0;

		// use request().await() to suspend regular behavior until MM responded
		auto mm = system().middleman().actor_handle();

		request(mm, infinite, connect_atom::value, host, port)
			.await
			(
				[host, port, this](const node_id&, strong_actor_ptr controller, const std::set<std::string>& ifs)
				{
					if (!controller)
					{
						debug_msg(R"(>>> no controller found at ")" + host + R"(":)" + std::to_string(port) + " <<<");
						return;
					}
					if (!ifs.empty())
					{
						debug_msg(R"(>>> typed actor found at ")" + host + R"(":)" + std::to_string(port) + ", but expected an untyped actor <<<");
						return;
					}
					debug_msg("successfully connected to controller");
					controller_host.assign(host);
					controller_port = port;
					this->controller = controller;
					auto controller_hdl = actor_cast<actor>(controller);
					this->monitor(controller_hdl);
					//this->send(controller_hdl, elevator::register_elevator_atom::value, this);
					transition_to_state(elevator_fsm::idle);
				},
				[host, port, this](const error& err)
				{
					debug_msg(R"(>>> cannot connect to ")" + host + R"(":)" + std::to_string(controller_port) + " => " + this->system().render(err) + " <<<");
					transition_to_state(elevator_fsm::disconnected);
				}
				);
	}

	// waypoint floor received from controller
	void elevator_actor::on_waypoint_received(int waypoint_floor)
	{
		if (waypoint_floor > elevator::FLOOR_MAX || waypoint_floor < elevator::FLOOR_MIN)
			return;
		waypoint_floors.emplace(waypoint_floor); // simple fifo behaviour for now
	}

	// no more waypoints/passengers, wait for a job from the controller
	void elevator_actor::on_idle()
	{
	}

	// start the lift if there are any waypoints
	// return true: has waypoints and ready to go, false: no waypoints
	// FSM only has idle_state calling this; it's just needed to kick off state transitions from idle if there are waypoints
	bool elevator_actor::on_start()
	{
		return true;
	}

	// starting moving
	void elevator_actor::on_in_transit()
	{
	}

	// at a floor, doors opening, picking up & dropping off passengers, 
	void elevator_actor::on_waypoint_arrive(int waypoint_floor)
	{
	}

	void elevator_actor::timer_pulse(int seconds)
	{
		delayed_send(this, std::chrono::seconds(seconds), elevator::timer_atom::value);
	}

	void elevator_actor::debug_msg(std::string msg)
	{
		aout(this) << "\n[elevator][" << name << "][" << fsm->get_state_name() << "][" << current_floor << "]: " << msg << "\n" << std::flush;
	}


}