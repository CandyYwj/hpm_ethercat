builtin_interfaces/Time stamp
uint32 sequence
# Bit n marks CAN motor ID n + 1 as valid. feedback[n] carries that ID.
uint32 valid_mask
MotorFeedback[16] feedback
