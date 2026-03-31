interface BatteryMonitorProps {
  name: string
  pcbX?: string
  pcbY?: string
}

export const BatteryMonitor = ({
  name,
  pcbX,
  pcbY,
}: BatteryMonitorProps) => (
  <group name={name} pcbX={pcbX} pcbY={pcbY}>
    <resistor
      name="R6"
      resistance="100k"
      footprint="0402"
      pcbX="0mm"
      pcbY="0mm"
      connections={{ pin1: "net.VBAT", pin2: "net.ADC_BATT" }}
    />
    <resistor
      name="R7"
      resistance="220k"
      footprint="0402"
      pcbX="0mm"
      pcbY="2mm"
      connections={{ pin1: "net.ADC_BATT", pin2: "net.GND" }}
    />
    <capacitor
      name="C8"
      capacitance="100nF"
      footprint="0402"
      pcbX="2mm"
      pcbY="1mm"
      connections={{ pin1: "net.ADC_BATT", pin2: "net.GND" }}
    />
  </group>
)
