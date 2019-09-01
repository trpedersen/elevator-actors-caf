#pragma once

//#include "passenger_actor.hpp"

namespace passenger
{


	// State transition for the passenger for connecting to the elevator controller, making calls, in transit, etc:
	//
	//                    +-------------+
	//                    |    init     |
	//                    +-------------+
	//                           |
	//                           V
	//                    +-------------+
	//                    | unconnected |<------------------+
	//                    +-------------+                   |
	//                           |                          |
	//                           | {connect Host Port}      |
	//                           |                          |
	//                           V                          |
	//                    +-------------+  {error}          |
	//    +-------------->| connecting  |-------------------+
	//    |               +-------------+                   |
	//    |                      |                          |
	//    |                      | {ok, connected}          |
	//    |{connect Host Port}   |                          |
	//    |                      V                          |
	//    |               +-------------+ {DOWN controller} |
	//    +---------------|   waiting   |-------------------+
	//                    | for instruction|<------+       
	//	                  +-------------+          |
	//	                        |                  |
	//       {lift arrives}     |                  | {arrived at destination/disembark}
	//                          V                  |
	//                    +-------------+          |
	//                    | in lift/    |          |
	//                    | in transit  |----------+
	//                    +-------------+

	class passenger_actor;

	enum class passenger_event_type
	{
		initiate,
		initialised_ok,
		connect,
		connected_ok,
		connection_fail,
		disconnect,
		disconnected,
		call,
		elevator_arrived,
		destination_arrived,
		quit
	};

	struct passenger_event
	{
		const passenger_actor& actor;
		passenger_event_type event_type;
	};


	class initialising_state;
	class disconnected_state;
	class connecting_state;
	class in_lobby_state;
	class in_elevator_state;
	class awaiting_instruction_state;
	class quitting_state;

	class passenger_state
	{
	public:
		//virtual ~passenger_state() {};
		virtual void on_enter(passenger_actor& actor) {};
		virtual void on_exit(passenger_actor& actor) {};
		virtual void handle_event(passenger_actor& actor, const passenger_event& event) {};

		static std::shared_ptr<initialising_state> initalising;
		static std::shared_ptr<disconnected_state> disconnected;
		static std::shared_ptr<connecting_state> connecting;
		static std::shared_ptr<in_lobby_state> in_lobby;
		static std::shared_ptr<in_elevator_state> in_elevator;
		static std::shared_ptr<awaiting_instruction_state> awaiting_instruction;
		static std::shared_ptr<quitting_state> quitting;
	};

	class initialising_state : public passenger_state
	{
		virtual void on_enter(passenger_actor& actor) override;
		virtual void handle_event(passenger_actor& actor, const passenger_event& event) override;
	};

	class disconnected_state : public passenger_state
	{
		void on_enter(passenger_actor& actor) override;
		virtual void  handle_event(passenger_actor& actor, const passenger_event& event) override;
	};

	class connecting_state : public passenger_state
	{
		void on_enter(passenger_actor& actor) override;
		virtual void  handle_event(passenger_actor& actor, const passenger_event& event) override;
	};

	class in_lobby_state : public passenger_state
	{
		void on_enter(passenger_actor& actor) override;
		virtual void handle_event(passenger_actor& actor, const passenger_event& event) override;
	};

	class in_elevator_state : public passenger_state
	{
		void on_enter(passenger_actor& actor) override;
		virtual void handle_event(passenger_actor& actor, const passenger_event& event) override;
	};

	class awaiting_instruction_state : public passenger_state
	{
		void on_enter(passenger_actor& actor) override;
		virtual void handle_event(passenger_actor& actor, const passenger_event& event) override;
	};

	class quitting_state : public passenger_state
	{
		virtual void on_enter(passenger_actor& actor) override;
	};





}
