interface PowerManagementProps {
  name: string
  pcbX?: string
  pcbY?: string
}

export const PowerManagement = ({
  name,
  pcbX,
  pcbY,
}: PowerManagementProps) => (
  <group name={name} pcbX={pcbX} pcbY={pcbY}>
    {/* ===== TP4056 Li-Ion Charger ===== */}
    <chip
      name="U3"
      manufacturerPartNumber="TP4056"
      footprint="soic8"
      pcbX="0mm"
      pcbY="0mm"
      pinLabels={{
        1: "TEMP",
        2: "PROG",
        3: "GND",
        4: "VCC",
        5: "STDBY",
        6: "CHRG",
        7: "BAT",
        8: "CE",
      }}
      schPinArrangement={{
        leftSide: {
          pins: [4, 8, 1, 2],
          direction: "top-to-bottom",
        },
        rightSide: {
          pins: [7, 6, 5, 3],
          direction: "top-to-bottom",
        },
      }}
      connections={{
        VCC: "net.CHARGE_IN",
        GND: "net.GND",
        BAT: "net.VBAT_RAW",
        CE: "net.CHARGE_IN",
      }}
    />

    <resistor
      name="R3"
      resistance="1.2k"
      footprint="0402"
      pcbX="-1mm"
      pcbY="3mm"
      connections={{ pin1: ".U3 > .PROG", pin2: "net.GND" }}
    />
    <capacitor
      name="C3"
      capacitance="10uF"
      footprint="0805"
      pcbX="-4mm"
      pcbY="0mm"
      connections={{ pin1: "net.CHARGE_IN", pin2: "net.GND" }}
    />
    <capacitor
      name="C4"
      capacitance="10uF"
      footprint="0805"
      pcbX="4mm"
      pcbY="0mm"
      connections={{ pin1: "net.VBAT_RAW", pin2: "net.GND" }}
    />

    {/* LEDs near TP4056 */}
    <resistor
      name="R4"
      resistance="1k"
      footprint="0402"
      pcbX="-6mm"
      pcbY="3mm"
      connections={{ pin1: ".U3 > .CHRG", pin2: "net.LED_CHRG" }}
    />
    <led
      name="LED1"
      color="red"
      footprint="0603"
      pcbX="-6mm"
      pcbY="5mm"
      connections={{ anode: "net.LED_CHRG", cathode: "net.GND" }}
    />
    <resistor
      name="R5"
      resistance="1k"
      footprint="0402"
      pcbX="-4mm"
      pcbY="3mm"
      connections={{ pin1: ".U3 > .STDBY", pin2: "net.LED_STDBY" }}
    />
    <led
      name="LED2"
      color="green"
      footprint="0603"
      pcbX="-4mm"
      pcbY="5mm"
      connections={{ anode: "net.LED_STDBY", cathode: "net.GND" }}
    />

    <trace path={[".U3 > .TEMP", "net.GND"]} />

    {/* ===== DW01A + 8205A — compact, stacked vertically ===== */}
    <chip
      name="U4"
      manufacturerPartNumber="DW01A"
      footprint="sot23_6"
      pcbX="6mm"
      pcbY="5mm"
      pinLabels={{
        1: "OD",
        2: "CS",
        3: "OC",
        4: "TD",
        5: "VCC",
        6: "GND",
      }}
      schPinArrangement={{
        leftSide: {
          pins: [5, 4, 6],
          direction: "top-to-bottom",
        },
        rightSide: {
          pins: [1, 3, 2],
          direction: "top-to-bottom",
        },
      }}
      connections={{
        VCC: "net.VBAT_RAW",
        GND: "net.BATT_NEG",
      }}
    />

    <chip
      name="U5"
      manufacturerPartNumber="8205A"
      footprint="sot23_6"
      pcbX="6mm"
      pcbY="9mm"
      pinLabels={{
        1: "S1",
        2: "G1",
        3: "S2",
        4: "D",
        5: "G2",
        6: "D2",
      }}
      schPinArrangement={{
        leftSide: {
          pins: [2, 1, 3],
          direction: "top-to-bottom",
        },
        rightSide: {
          pins: [5, 4, 6],
          direction: "top-to-bottom",
        },
      }}
    />

    <trace path={[".U4 > .OD", ".U5 > .G1"]} />
    <trace path={[".U4 > .OC", ".U5 > .G2"]} />
    <trace path={[".U4 > .CS", ".U5 > .S1"]} />
    <trace path={[".U5 > .S1", ".U5 > .S2"]} />
    <trace path={[".U5 > .D", "net.GND"]} />
    <trace path={[".U5 > .D2", "net.GND"]} />

    {/* Battery connector next to protection ICs */}
    <pinheader
      name="J1"
      pinCount={2}
      pitch="2mm"
      pcbX="10mm"
      pcbY="7mm"
      pinLabels={{
        1: "BATT_POS",
        2: "BATT_NEG",
      }}
      connections={{
        pin1: "net.VBAT_RAW",
        pin2: "net.BATT_NEG",
      }}
    />

    <trace path={["net.VBAT_RAW", "net.VBAT"]} />

    {/* ===== LDO — close to TP4056 output ===== */}
    <chip
      name="U6"
      manufacturerPartNumber="AP2112K-3.3"
      footprint="sot23_5"
      pcbX="0mm"
      pcbY="-5mm"
      pinLabels={{
        1: "VIN",
        2: "GND",
        3: "EN",
        4: "NC",
        5: "VOUT",
      }}
      schPinArrangement={{
        leftSide: {
          pins: [1, 3, 2],
          direction: "top-to-bottom",
        },
        rightSide: {
          pins: [5, 4],
          direction: "top-to-bottom",
        },
      }}
      connections={{
        VIN: "net.VBAT",
        GND: "net.GND",
        EN: "net.VBAT",
        VOUT: "net.V3V3",
      }}
    />

    <capacitor
      name="C5"
      capacitance="1uF"
      footprint="0402"
      pcbX="-3mm"
      pcbY="-5mm"
      connections={{ pin1: "net.VBAT", pin2: "net.GND" }}
    />
    <capacitor
      name="C6"
      capacitance="1uF"
      footprint="0402"
      pcbX="3mm"
      pcbY="-5mm"
      connections={{ pin1: "net.V3V3", pin2: "net.GND" }}
    />
  </group>
)
