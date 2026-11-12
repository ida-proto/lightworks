#include "app.hpp"

#include "status_led.hpp"
#include "usart3_vcp.hpp"

#include "lightworks/link.hpp"

#include "sbus/sbus.h"

namespace {

void send_boot(lightworks::Link<lightworks::h743::Usart3Vcp>& link) {
  sbus_msg_t msg;
  sbus_msg_clear(&msg);
  const sbus_heartbeat_t hb{SBUS_HB_BOOT, 0u, 0u, 0};
  if (!sbus_pack_heartbeat(&msg, &hb)) {
    return;
  }
  msg.version = SBUS_PROTO_MAJOR;
  msg.src = SBUS_NODE_GLOW_FIRST;
  msg.dst = SBUS_NODE_BROADCAST;
  msg.prio = sbus_default_prio(msg.type);
  (void)link.send(msg);
}

}  // namespace

int main() {
  lightworks::h743::status_led::init();

  // The app names the concrete pipe. USART3 is the ST-LINK VCP.
  lightworks::h743::Usart3Vcp vcp;
  vcp.init();
  lightworks::Link<lightworks::h743::Usart3Vcp> link(vcp);
  send_boot(link);

  lightworks::app::run();
}
