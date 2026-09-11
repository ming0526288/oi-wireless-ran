/*
 * External sensing-assisted UL MCS control.
 */

#include "external_ul_mcs_control.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common/utils/LOG/log.h"

#define EXTERNAL_UL_MCS_CONTROL_PORT 6000
#define EXTERNAL_UL_MCS_DEFAULT_MAX 28
#define EXTERNAL_UL_MCS_STATE_A 14
#define EXTERNAL_UL_MCS_STATE_B 25
#define EXTERNAL_UL_MCS_DECREASE_INTERVAL_SLOTS 80

static atomic_bool control_started = ATOMIC_VAR_INIT(false);
static atomic_bool control_active = ATOMIC_VAR_INIT(false);
static atomic_uint target_ul_max_mcs = ATOMIC_VAR_INIT(EXTERNAL_UL_MCS_DEFAULT_MAX);
static atomic_uint current_ul_max_mcs = ATOMIC_VAR_INIT(EXTERNAL_UL_MCS_DEFAULT_MAX);
static atomic_uint last_sched_tick = ATOMIC_VAR_INIT(UINT32_MAX);
static atomic_uint slots_since_mcs_step = ATOMIC_VAR_INIT(0);

static void apply_external_state(char state)
{
  uint8_t target;
  if (state == 'a')
    target = EXTERNAL_UL_MCS_STATE_A;
  else if (state == 'b')
    target = EXTERNAL_UL_MCS_STATE_B;
  else
    return;

  const bool was_active = atomic_exchange(&control_active, true);
  const uint8_t current = atomic_load(&current_ul_max_mcs);

  atomic_store(&target_ul_max_mcs, target);
  atomic_store(&slots_since_mcs_step, 0);

  if (!was_active || target >= current)
    atomic_store(&current_ul_max_mcs, target);

  LOG_I(NR_MAC, "[External Control] Received state: %c\n", state);
  LOG_I(NR_MAC,
        "[NR_MAC] Now max_mcs is %u (target %u)\n",
        atomic_load(&current_ul_max_mcs),
        target);
}

static void handle_external_client(int client_fd)
{
  char buf[64];

  while (true) {
    const ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
    if (n == 0)
      break;
    if (n < 0) {
      perror("[External Control] recv");
      break;
    }

    for (ssize_t i = 0; i < n; i++)
      apply_external_state(buf[i]);
  }
}

static void *external_control_thread(void *arg)
{
  (void)arg;

  const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("[External Control] socket");
    return NULL;
  }

  const int enable = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
    perror("[External Control] setsockopt");

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(EXTERNAL_UL_MCS_CONTROL_PORT);

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("[External Control] bind");
    close(server_fd);
    return NULL;
  }

  if (listen(server_fd, 5) < 0) {
    perror("[External Control] listen");
    close(server_fd);
    return NULL;
  }

  LOG_I(NR_MAC, "[External Control] Listening on TCP port %d\n", EXTERNAL_UL_MCS_CONTROL_PORT);

  while (true) {
    const int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
      if (errno == EINTR)
        continue;
      perror("[External Control] accept");
      continue;
    }

    handle_external_client(client_fd);
    close(client_fd);
  }

  close(server_fd);
  return NULL;
}

int start_external_ul_mcs_control(void)
{
  if (atomic_exchange(&control_started, true))
    return 0;

  pthread_t thread;
  const int ret = pthread_create(&thread, NULL, external_control_thread, NULL);
  if (ret != 0) {
    atomic_store(&control_started, false);
    fprintf(stderr, "[External Control] pthread_create failed: %s\n", strerror(ret));
    return -1;
  }

  pthread_detach(thread);
  return 0;
}

bool external_ul_mcs_control_is_active(void)
{
  return atomic_load(&control_active);
}

uint8_t get_external_ul_max_mcs(frame_t frame, sub_frame_t slot)
{
  const uint8_t target = atomic_load(&target_ul_max_mcs);
  uint8_t current = atomic_load(&current_ul_max_mcs);

  if (current == target)
    return current;

  const uint32_t tick = (frame << 16) | (slot & 0xffff);
  const uint32_t last_tick = atomic_load(&last_sched_tick);
  if (tick == last_tick)
    return current;

  atomic_store(&last_sched_tick, tick);

  if (current < target) {
    current = target;
    atomic_store(&current_ul_max_mcs, current);
    atomic_store(&slots_since_mcs_step, 0);
    LOG_I(NR_MAC, "[NR_MAC] Now max_mcs is %u (target %u)\n", current, target);
    return current;
  }

  const uint32_t slots = atomic_fetch_add(&slots_since_mcs_step, 1) + 1;
  if (slots >= EXTERNAL_UL_MCS_DECREASE_INTERVAL_SLOTS) {
    current--;
    atomic_store(&current_ul_max_mcs, current);
    atomic_store(&slots_since_mcs_step, 0);
    LOG_I(NR_MAC, "[NR_MAC] Now max_mcs is %u (target %u)\n", current, target);
  }

  return current;
}
