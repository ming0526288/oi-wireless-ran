# TEST-687 100 MHz 门限 0 接入和 ping 核验

构建和测试对象为 `e38538b86b`，后续记录提交只含 Markdown。构建在 `cmake_targets` 使用 `./build_oai -w DPDKRF --gNB --nrUE --ninja -d ran_build_dpdkrf`。首轮 ccache 在沙箱中无法创建临时目录，退出 1。保留日志，前置 `CCACHE_DISABLE=1` 后重跑退出 0，得到 gNB、UE、RFsim 和 DPDKRF 产物。`ldd` 成功解析设备库。本机未运行实际射频。

复用 TEST-332 的短 MAC 头 C 测试，通过 65,790 个长度与预留组合、65,546 个容量和 297 个精确大小缓冲区检查。命令文档的 11 段 Bash 语法通过。

## RFsim 测试

复用原核心网、三个命名空间和各自已登记的 UE 配置。gNB、各 UE 在构建目录的独立交互终端直接启动。gNB 命令与命令页相同，仅使用非交互 `sudo -n`。UE 按已有测试方法进入 `s2m56u0`、`s2m56u1` 和 `s2m56u2`，分别连 `10.203.1.1`、`10.203.2.1`、`10.203.3.1`，接收增益 20。依次测 1 UE、2 UE，随后加入第三台查共同接入。每台目标每 20 ms 发 56 B ping，经 `oaitun_ue1` 到 `172.11.200.25`。双 UE 业务同期进行。

```bash
sudo -n ip netns exec s2m56u0 ping -D -n -I oaitun_ue1 -c 200 -s 56 -i 0.02 -W 2 172.11.200.25
```

抓包用既有 `step2-multiue56-v1/ping_capture.py`，取每组同一批事务的 TUN 与 N3，校核上下行均为 QFI 9。每台统计前剔除 20 个预热序号。RTT 取自原生 ping。

| 条件 | 每台发出／收到 | 平均 RTT（ms） | P99（ms） |
|---|---:|---:|---:|
| 1 UE | 200／200 | 0.814 | 1.160 |
| 2 UE | 各 200／200 | 1.662／1.656 | 2.356／2.286 |
| 3 UE | 业务未开始 | 未测 | 未测 |

三 UE 组的第三台完成过随机接入竞争解决，接着多次重试，未配置业务地址。gNB 最后在 `nr_generate_Msg4()` 的 `tb_size >= pdu_length` 检查退出，另外两个连接随基站退出断开。三 UE 组未开始 ping 或抓包。这次未开启逐调度探针，无法用本批数据确定 Msg4 分配失败前每个调度机会的资源归因。

宿主复核无本次 RAN、监听端口、抓包进程或 UE TUN 残留，原三核心容器启动时间一致。完整构建日志、TUN／N3 抓包、ping 输出及三 UE 终端节选保存在本机 `analysis-source-audit/gate0-100MHz-base-20261125/`。
