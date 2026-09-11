# 120 kHz single-host RFsim with LatSeq

This branch combines the existing gNB LatSeq probes with UE-side probes in the
same OIWireless revision.  It is intended for a single-host, SA, Band 261,
120 kHz, 66 PRB RFsim smoke test before running the real DPDK/USRP setup.

The RFsim-only changes in
`targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb1.sa.band261.fr2.66PRB.5slot.dpdk.conf`
are:

- Band 261 and 120 kHz numerology (`subcarrierSpacing = 3`)
- `if_freq = 0`, so RFsim uses the configured carrier directly
- `sl_ahead = 12`, to give a single host enough scheduling margin
- `PARALLEL_SINGLE_THREAD`, to avoid the real-RU thread split in RFsim
- an RFsim server block on TCP port 4043

These settings are isolated on the `latseq-120k-rfsim` branch.  Do not use this
configuration as the final DPDK/USRP configuration.

## 1. Build

```bash
cd ~/oiwireless/latseq-120k-rfsim/cmake_targets
mkdir -p /tmp/latseq-120k-ccache
CCACHE_TEMPDIR=/tmp/latseq-120k-ccache \
  ./build_oai -w SIMU --ninja --gNB --nrUE --enable-latseq
```

## 2. Start the 5GC

```bash
cd ~/oiwireless/oiwireless-5gc-deploy
./run-5gc.sh
```

DPDK initialization is not needed for this RFsim test.

## 3. Start the gNB

Open another terminal:

```bash
cd ~/oiwireless/latseq-120k-rfsim/cmake_targets/ran_build/build
sudo ./nr-softmodem \
  -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb1.sa.band261.fr2.66PRB.5slot.dpdk.conf \
  --sa --rfsim --latseq_ul 1
```

## 4. Start the UE

Open another terminal after the gNB is listening:

```bash
cd ~/oiwireless/latseq-120k-rfsim/cmake_targets/ran_build/build
sudo ./nr-uesoftmodem \
  --band 261 -C 27547560000 -r 66 --numerology 3 --ssb 301 \
  --sa --rfsim --rfsimulator.serveraddr 127.0.0.1 \
  -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.oiwireless.conf \
  --latseq_ul 1
```

Wait for registration and PDU-session establishment.  Confirm the tunnel and
send traffic from a fourth terminal:

```bash
ip -brief addr show oaitun_ue1
ping -I oaitun_ue1 -c 10 192.169.0.1
```

Stop the UE with Ctrl-C first, then stop the gNB with Ctrl-C.  Graceful shutdown
flushes the LatSeq buffers to `/tmp/nr_uesoftmodem.*.lseq` and
`/tmp/nr_softmodem.*.lseq`.

## 5. Convert and validate

Select the two files created by the same run, then run:

```bash
mkdir -p ~/oiwireless/latseq-results/my-120k-test
cd ~/oiwireless/latseq-analysis

python3 rdtsctots.py /tmp/nr_softmodem.YOUR_RUN.lseq \
  > ~/oiwireless/latseq-results/my-120k-test/bs.lseq
python3 rdtsctots.py /tmp/nr_uesoftmodem.YOUR_RUN.lseq \
  > ~/oiwireless/latseq-results/my-120k-test/ue.lseq

python3 validate_smoke.py \
  ~/oiwireless/latseq-results/my-120k-test/ue.lseq \
  ~/oiwireless/latseq-results/my-120k-test/bs.lseq

python3 calculate_ping_latencies.py \
  ~/oiwireless/latseq-results/my-120k-test/ue.lseq \
  ~/oiwireless/latseq-results/my-120k-test/bs.lseq \
  --csv ~/oiwireless/latseq-results/my-120k-test/t1-t12.csv
```

The smoke test performed on 2026-09-11 completed 10 of 10 packet paths.  Its
RFsim uplink `t12-t1` mean was 0.797 ms, median 0.748 ms, minimum 0.567 ms, and
maximum 1.206 ms.  RFsim timing is a functional baseline, not a substitute for
the later DPDK/USRP latency measurement.
