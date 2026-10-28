# Home button on Mi 6

Goal: reach a state where the iOS UI is past the lock screen, so the graphics
work in this repository can actually be looked at.

## What the target identity expects

From the D20AP device tree in `20H392`:

```
buttons {
    compatible            = "buttons"
    home-button-type      = <1>
    button-delay_hold     = <75000>
    opposed-power-vol-buttons = <1>
    function-button_volup_pre_p2   = ...
    function-button_voldown_pre_p2 = ...
    function-button_ringerab       = ...
    interrupt-parent_pre_p2 = <0x24>
    supports-trm
}
```

`home-button-type = <1>` is the solid-state variant. The iPhone 8 has no
clicking Home button; a force sensor reports contact and the Taptic Engine
fakes the click. So the stack we are impersonating is built to consume
"sensor says finger is here", not "switch closed".

Mi 6 is the same shape of problem: the only Home-area sensor is the fingerprint
sensor under the glass, and it reports finger contact. The analogy is real, and
that is why this port is worth doing at all.

## What the system consumes

The Home action reaches SpringBoard as a HID event, Consumer page `0x0C`,
usage `0x40` (`Menu`). That is distinct from the keyboard `Home` key and from
`AC Home`, and it is what an accessibility switch or an external keyboard sends
when it presses Home.

A minimal report is one byte: bit 0 asserted while the finger is down, cleared
on release. Releasing matters: a Home that never releases leaves the system in
a press-and-hold state.

## The paths, cheapest first

### Path 0 — AssistiveTouch, no driver at all

Enable `AssistiveTouchEnabled` in the accessibility preferences on the data
volume before boot. The on-screen Home control appears over the lock screen and
is driven by the touchscreen, which already works.

Cost: a plist. Risk: whether SpringBoard honours a pre-seeded preference on a
fresh boot is unverified.

This is the fastest way to answer "does the UI actually come up", and it is
independent of everything below.

### Path 1 — describe an existing key as Home

Volume-down arrives on PM8998 GPIO6. If we declare it as the home button in the
device tree and Apple's native button driver binds, Home works with no new
driver code at all. The cost is losing volume-down.

Cost: a device-tree patch. Risk: depends on the native button driver existing
and matching on our platform, which is unknown.

### Path 2 — our own HID device

A small `IOHIDDevice` that publishes Consumer `0x0C`/`0x40`, fed from PM8998
GPIO6 (volume-down), or from a bottom-centre tap region on the touchscreen as a
second source. No secure world involved.

Cost: one driver, one event source. Risk: low, and it is a fallback that does
not depend on Apple's drivers.

### Path 3 — the real thing: the fingerprint sensor

`FP_INT_N` on GPIO121 only says "something happened". What happened is read
over SPI through QSEE, which needs the vendor trusted application loaded by
firmware that initialises the secure world first.

Cost: firmware extraction, TA loading, SMC transport, plus the sensor protocol.
This is the correct long-term answer and the one that gives a real Home button,
but it is weeks, not an evening, and it depends on the boot path already working.

## Order

Path 0 first, purely to see the UI. Then Path 1 as a one-shot experiment. If it
does not bind, Path 2. Path 3 belongs after the graphics work is showing frames,
not before.
