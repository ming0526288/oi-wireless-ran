# 120 kHz LatSeq probes-only branch

This branch is based directly on `383aa52a31` (`before-latseq`). It adds the
LatSeq recorder, uplink probes, and the `--latseq_ul` run-time switch. It does
not change the original Band 261 configuration, scheduler, RLC behavior,
threading mode, RFsim topology, gain options, or traffic destination.

## Build

For RFsim:

```bash
cd ~/oiwireless/oi-wireless-ran/cmake_targets
./build_oai -w SIMU --ninja --gNB --nrUE --enable-latseq
```

For the real DPDK gNB, rebuild with the original radio backend:

```bash
cd ~/oiwireless/oi-wireless-ran/cmake_targets
./build_oai -w DPDKRF --ninja --gNB --enable-latseq
```

## Strict A/B rule

Use exactly the original command line for the OFF run. For the ON run, add
only this option to both gNB and UE:

```text
--latseq_ul 1
```

With the option omitted (its default is zero), LatSeq does not initialize and
does not create a trace file. With it enabled, traces are written under `/tmp`
as `nr_softmodem.*.lseq` and `nr_uesoftmodem.*.lseq`.

Keep the original RFsim server address, UE gain/frequency-compensation options,
configuration files, and `ping 172.11.200.25 -I oaitun_ue1` unchanged between
the two runs.
