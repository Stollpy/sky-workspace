interface ProgrammingHeaderProps {
  name: string
  pcbX?: string
  pcbY?: string
}

export const ProgrammingHeader = ({
  name,
  pcbX,
  pcbY,
}: ProgrammingHeaderProps) => (
  <group name={name} pcbX={pcbX} pcbY={pcbY}>
    <testpoint
      name="TP1"
      footprintVariant="pad"
      padShape="circle"
      padDiameter="1.5mm"
      pcbX="0mm"
      pcbY="0mm"
      connections={{ pin1: "net.TX" }}
    />
    <testpoint
      name="TP2"
      footprintVariant="pad"
      padShape="circle"
      padDiameter="1.5mm"
      pcbX="2.5mm"
      pcbY="0mm"
      connections={{ pin1: "net.RX" }}
    />
    <testpoint
      name="TP3"
      footprintVariant="pad"
      padShape="circle"
      padDiameter="1.5mm"
      pcbX="5mm"
      pcbY="0mm"
      connections={{ pin1: "net.GND" }}
    />
    <testpoint
      name="TP4"
      footprintVariant="pad"
      padShape="circle"
      padDiameter="1.5mm"
      pcbX="7.5mm"
      pcbY="0mm"
      connections={{ pin1: "net.V3V3" }}
    />
    <testpoint
      name="TP5"
      footprintVariant="pad"
      padShape="circle"
      padDiameter="1.5mm"
      pcbX="10mm"
      pcbY="0mm"
      connections={{ pin1: "net.EN" }}
    />
    <testpoint
      name="TP6"
      footprintVariant="pad"
      padShape="circle"
      padDiameter="1.5mm"
      pcbX="12.5mm"
      pcbY="0mm"
      connections={{ pin1: "net.GPIO0" }}
    />
  </group>
)
