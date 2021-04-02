{
    // Make a conf object for TimingTriggerCandidateMaker
    conf(s0_signal_type=0,
	s0_time_before=10000,
	s0_time_after=20000,
    	s1_signal_type=1,
	s1_time_before=100000,
	s1_time_after=200000,
    	s2_signal_type=2,
	s2_time_before=1000000,
	s2_time_after=2000000) :: {

	s0: [s0_signal_type, s0_time_before, s0_time_end],
	s1: [s1_signal_type, s1_time_before, s1_time_end],
	s2: [s2_signal_type, s2_time_before, s2_time_end],
    },
}
