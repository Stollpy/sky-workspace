interface Esp32ModuleProps {
  name: string
  pcbX?: string
  pcbY?: string
}

export const Esp32Module = ({ name, pcbX, pcbY }: Esp32ModuleProps) => (
  <group name={name} pcbX={pcbX} pcbY={pcbY}>
    <chip
      name="U1"
      manufacturerPartNumber="ESP32-WROOM-32"
      footprint="stampboard28_w15.24mm_p1.27mm_pw0.6mm_pl1.6mm"
      pcbX="0mm"
      pcbY="0mm"
      pinLabels={{
        1: "GND1",
        2: "V3V3",
        3: "EN",
        4: "GPIO36",
        5: "GPIO39",
        6: "GPIO34",
        7: "GPIO35",
        8: "GPIO32",
        9: "GPIO33",
        10: "GPIO25",
        11: "GPIO26",
        12: "GPIO27",
        13: "GPIO14",
        14: "GPIO12",
        15: "GPIO13",
        16: "GND2",
        17: "GPIO15",
        18: "GPIO2",
        19: "GPIO0",
        20: "GPIO4",
        21: "GPIO16",
        22: "GPIO17",
        23: "GPIO5",
        24: "GPIO18",
        25: "GPIO19",
        26: "GPIO21",
        27: "GPIO3_RX",
        28: "GPIO1_TX",
        29: "GPIO22",
        30: "GPIO23",
        31: "GND3",
      }}
      schPinArrangement={{
        leftSide: {
          pins: [2, 3, 19, 6, 15, 14, 13, 12],
          direction: "top-to-bottom",
        },
        rightSide: {
          pins: [28, 27, 30, 29, 26, 25, 24, 23],
          direction: "top-to-bottom",
        },
        bottomSide: {
          pins: [1, 16, 31],
          direction: "left-to-right",
        },
      }}
      connections={{
        GND1: "net.GND",
        GND2: "net.GND",
        GND3: "net.GND",
        V3V3: "net.V3V3",
      }}
    />

    {/* Decoupling caps adjacent to VCC pin */}
    <capacitor
      name="C1"
      capacitance="10uF"
      footprint="0805"
      pcbX="-10mm"
      pcbY="-3mm"
      connections={{ pin1: "net.V3V3", pin2: "net.GND" }}
    />
    <capacitor
      name="C2"
      capacitance="100nF"
      footprint="0402"
      pcbX="-10mm"
      pcbY="-1mm"
      connections={{ pin1: "net.V3V3", pin2: "net.GND" }}
    />

    {/* Pull-up EN + filter */}
    <resistor
      name="R1"
      resistance="10k"
      footprint="0402"
      pcbX="-10mm"
      pcbY="1mm"
      connections={{ pin1: "net.V3V3", pin2: ["net.EN", ".U1 > .EN"] }}
    />
    <capacitor
      name="C7"
      capacitance="100nF"
      footprint="0402"
      pcbX="-10mm"
      pcbY="3mm"
      connections={{ pin1: "net.EN", pin2: "net.GND" }}
    />

    {/* Pull-up GPIO0 */}
    <resistor
      name="R2"
      resistance="10k"
      footprint="0402"
      pcbX="-10mm"
      pcbY="5mm"
      connections={{ pin1: "net.V3V3", pin2: ["net.GPIO0", ".U1 > .GPIO0"] }}
    />
  </group>
)
