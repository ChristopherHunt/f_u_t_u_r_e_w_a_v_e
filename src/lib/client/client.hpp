#ifndef __CLIENT__HPP__
#define __CLIENT__HPP__

#include <stdint.h>
#include <string>
#include "network/network.hpp"
#include <sys/time.h>
#include "portmidi/include/portmidi.h"
#include "portmidi/include/porttime.h"

#define MAX_TIMEOUTS 5
#define INPUT_ARG_COUNT 4

namespace client {
   enum Client_State { HANDSHAKE, TWIDDLE, PLAY, DONE };
};

// Function for seperate PortMidi thread to call to update timestamps within the
// client object.
void process_midi(PtTimestamp timestamp, void *userData);

class Client {
   private:
      // States of the file transfer state machine.
      client::Client_State state;   // State of the instrument node.

      sockaddr_in server;           // Server connection information.
      int server_sock;              // Socket for connecting to the server.
      uint32_t server_port;         // Port of server.
      std::string server_machine;   // Server's IP.

      int seq_num;                  // The current packet sequence number.
      fd_set rdfds;                 // Set of fds to select on.
      struct timeval tv;            // Timeval for select.
      uint8_t timeout_count;        // # times select has timed out in a row.

      long delay;                   // Simulated network delay
      int midi_channel;             // Target midi channel to play out of

      uint8_t buf[MAX_BUF_SIZE];    // Buffer used for message handling.

      PortMidiStream *stream;       // Pointer to the port midi output stream.
      int default_device_id;        // Default device id for this midi device.
      Packet_Header *midi_header;   // Header pointer for overlaying on midi messages.
      Packet_Header midi_ack;       // Structure used for acking midi messages.
      PmMessage message;            // Message to receive midi into.
      PmEvent event;                // Event to play the midi message.
      MyPmEvent *my_event;          // Event to send to output midi device.

      // Returns true if there's a response ready for receiving from the server.
      int check_for_response(uint32_t timeout);

      // Checks if the current select_timeout is equal to the max allowable
      // timeouts, and if it is is prints an error message and exits.
      void check_timeout();

      // Configures a fresh FD_SET that contains the server_sock.
      void config_fd_set();

      // Handles the exit message from the server, closing the client.
      void handle_done();

      // Handles the setup of the client with the server.
      void handle_handshake();

      // Parses the midi data sent to the client from the server.
      void handle_midi_data();

      // Handles the playing of the song's midi events from the server.
      void handle_play();

      // Handles sync messages between the client and the server.
      void handle_sync();

      // Parses a handshake ack, returning its flag.
      flag::Packet_Flag parse_handshake_ack();

      // Parses a list of arguments which correspond to the required
      // configuration options for the client class. If any errors are
      // detected, a message will be printed and the program will exit.
      bool parse_inputs(int num_args, char **arg_list);

      // Prints the usage message specifying the input arguments to the client
      // constructor.
      void print_usage();

      // Drops the client into the state machine to connect with the server and
      // play a song.
      void ready_go();

      // Recv's the contents of a UDP message into the client's message buffer
      // for futher processing by other functions. Also sets the Client's
      // seq_num to be 1 greater than that of the recv'd packet. Returns the
      // number of bytes received.
      int recv_packet_into_buf(uint32_t packet_size);

      // Assemble and send the handshake packet to the server.
      void send_handshake();

      // Sends the ack to the server so that the server knows the client is
      // ready to go!
      void send_handshake_fin();

      // Increments the sequence number and sends an ack to the server for a
      // midi message.
      void send_midi_ack();

      // Sets tv to have timeout seconds.
      void set_timeval(uint32_t timeout);

      // Sets up the client's socket to connect to the server on.
      void setup_udp_socket();

      // Handles the waiting state of the client when it is sitting around for
      // instructions from the server.
      void twiddle();

   public:
      PtTimestamp midi_timer; // Midi timer (uint32_t)

      // Base constructor, takes in a list of arguments and their count to be
      // parsed and used for the filetransfer.
      Client(int num_args, char **arg_list);

      // Base destructor.
      ~Client();
};

#endif
