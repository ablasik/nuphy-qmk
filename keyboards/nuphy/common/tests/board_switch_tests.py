from pathlib import Path
r=Path.cwd(); out=r/'.build/connection-actions-tests'
for board in ['air60_v2','air75_v2','air96_v2']:
 s=(r/'keyboards/nuphy'/board/'ansi/user_kb.c').read_text()
 def function(name):
  start=s.index('void '+name+'('); opening=s.index('{',start); level=1; end=opening+1
  while level:
   if s[end]=='{': level+=1
   if s[end]=='}': level-=1
   end+=1
  return s[start:end]
 latch=s[s.index('static bool requested_link_pending'):s.index('/**\n * @brief  long press key process.')]
 fixture=Path('keyboards/nuphy/common/tests/board_switch_fixture.c').read_text()
 fixture=fixture.replace('/* PRODUCTION_ADAPTER */',latch+'\n'+function('switch_dev_link')+'\n'+function('dial_sw_scan'))
 (out/(board+'-switch.c')).write_text(fixture)

# Exercise the production ChibiOS event callback and queue with real OS detection.
s=(r/'tmk_core/protocol/chibios/usb_main.c').read_text()
start=s.index('#define USB_EVENT_QUEUE_SIZE')
end=s.index('\n\n',s.index('\n}',s.index('static void usb_event_cb(')))
fixture=Path('keyboards/nuphy/common/tests/usb_os_fixture.c').read_text()
fixture=fixture.replace('/* PRODUCTION_USB_EVENTS */',s[start:end])
(out/'usb-os.c').write_text(fixture)
