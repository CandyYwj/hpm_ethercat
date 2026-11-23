# Experience report: RS06 MIT feedback investigation

Do not treat successful CAN initialization as evidence that a motor command was
transmitted. The motor manager intentionally gates CAN motion traffic on a
changed EtherCAT output-PDO command sequence. With device query and safe
feedback polling disabled, it also does not discover motors autonomously.

For RS06 MIT feedback, the standard CAN identifier is the feedback/host ID,
whereas the physical motor ID is carried in data byte 0. Do not hard-code one
global feedback ID: configure it per MIT axis and route accepted frames by bus
and payload motor ID. A CAN RX counter that rises while feedback remains zero
often indicates a decoder filter mismatch, not a failed receive ISR.
