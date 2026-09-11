# LatSeq real-gNB test (120 kHz, Band 261, DPDKRF)

This worktree preserves the current custom gNB baseline and adds the paper's
gNB-side uplink LatSeq points.  The existing configuration remains unchanged:
66 PRBs, 120 kHz SCS (`subcarrierSpacing = 3`), five-slot TDD pattern, and
`min_rxtxtime = 4`.

## 1. Build

```bash
cd ~/oiwireless/latseq-real-bs/cmake_targets
mkdir -p /tmp/latseq-real-ccache
CCACHE_TEMPDIR=/tmp/latseq-real-ccache \
  ./build_oai -w DPDKRF --ninja --gNB --enable-latseq
```

The executable is generated at:

```text
~/oiwireless/latseq-real-bs/cmake_targets/ran_build/build/nr-softmodem
```

## 2. Start the real system

Terminal 1, start the core network with the existing deployment:

```bash
cd ~/oiwireless/oiwireless-5gc-deploy
./run-5gc.sh
```

Initialize DPDK after a reboot:

```bash
sudo /home/lbh/init_dpdk.sh
```

Terminal 2, start this isolated LatSeq gNB build:

```bash
cd ~/oiwireless/latseq-real-bs/cmake_targets/ran_build/build
sudo ./nr-softmodem \
  -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb1.sa.band261.fr2.66PRB.5slot.dpdk.conf \
  --sa --latseq_ul 1
```

Do not use the executable under `~/oiwireless/oi-wireless-ran`; that is the
untouched baseline.  `--latseq_ul 1` is required to initialize and enable the
LatSeq buffers.  Stop the gNB normally with Ctrl-C so buffered entries are
written before the file is closed.

## 3. Generate uplink traffic

After the UE is registered and its tunnel is reachable, generate a bounded
test such as ten pings from the UE side.  Use the actual UE tunnel/interface
and core-side destination used by the real setup.

The raw gNB trace is written as:

```text
/tmp/nr_softmodem.DDMMYYYY_HHMMSS.lseq
```

## 4. Convert and validate t7-t12

First identify the trace belonging to this run:

```bash
ls -lhtr /tmp/nr_softmodem.*.lseq
```

Then substitute that exact raw filename below:

```bash
mkdir -p ~/oiwireless/latseq-results/real-120k
cd ~/oiwireless/latseq-analysis
python3 rdtsctots.py /tmp/nr_softmodem.DDMMYYYY_HHMMSS.lseq \
  > ~/oiwireless/latseq-results/real-120k/bs.lseq
python3 validate_real_bs.py \
  ~/oiwireless/latseq-results/real-120k/bs.lseq
```

A successful gNB trace contains the six uplink points corresponding to the
paper's gNB-side t7-t12 path:

```text
phy.ok -> mac.ts -> rlc.ts -> pdcp.ts -> sdap.ts.infos -> gtp.ts.infos
```

With a commercial or otherwise unmodified real UE, this test measures only
the gNB-side t7-t12 portion.  A complete t1-t12 latency needs LatSeq probes and
a synchronized trace on a modifiable OAI UE as well.
