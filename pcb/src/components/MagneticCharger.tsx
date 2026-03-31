interface MagneticChargerProps {
  name: string
  pcbX?: string
  pcbY?: string
}

export const MagneticCharger = ({
  name,
  pcbX,
  pcbY,
}: MagneticChargerProps) => (
  <group name={name} pcbX={pcbX} pcbY={pcbY}>
    <pinheader
      name="J3"
      pinCount={2}
      pitch="2.54mm"
      pcbX="0mm"
      pcbY="0mm"
      pinLabels={{
        1: "VCC_IN",
        2: "GND_IN",
      }}
      connections={{ pin2: "net.GND" }}
    />
    <diode
      name="D1"
      schottky
      footprint="sma"
      pcbX="4mm"
      pcbY="0mm"
      connections={{
        anode: ".J3 > .pin1",
        cathode: "net.CHARGE_IN",
      }}
    />
  </group>
)
