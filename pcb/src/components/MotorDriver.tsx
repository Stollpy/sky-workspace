interface MotorDriverProps {
  name: string
  pcbX?: string
  pcbY?: string
}

export const MotorDriver = ({ name, pcbX, pcbY }: MotorDriverProps) => (
  <group name={name} pcbX={pcbX} pcbY={pcbY}>
    <chip
      name="U2"
      manufacturerPartNumber="ULN2003A"
      footprint="soic16"
      pcbX="0mm"
      pcbY="0mm"
      pinLabels={{
        1: "IN1",
        2: "IN2",
        3: "IN3",
        4: "IN4",
        5: "IN5",
        6: "IN6",
        7: "IN7",
        8: "GND",
        9: "COM",
        10: "OUT7",
        11: "OUT6",
        12: "OUT5",
        13: "OUT4",
        14: "OUT3",
        15: "OUT2",
        16: "OUT1",
      }}
      schPinArrangement={{
        leftSide: {
          pins: [1, 2, 3, 4, 5, 6, 7, 8],
          direction: "top-to-bottom",
        },
        rightSide: {
          pins: [16, 15, 14, 13, 12, 11, 10, 9],
          direction: "top-to-bottom",
        },
      }}
      connections={{
        IN1: "net.MOTOR_IN1",
        IN2: "net.MOTOR_IN2",
        IN3: "net.MOTOR_IN3",
        IN4: "net.MOTOR_IN4",
        GND: "net.GND",
        COM: "net.VBAT",
      }}
    />

    {/* J2 vertical (rotated 90°) — fits within board, right edge */}
    <pinheader
      name="J2"
      pinCount={5}
      pitch="2.5mm"
      pcbX="6mm"
      pcbY="0mm"
      pcbRotation="90deg"
      pinLabels={{
        1: "A_POS",
        2: "A_NEG",
        3: "B_POS",
        4: "B_NEG",
        5: "VCC",
      }}
    />

    <trace path={[".U2 > .OUT1", ".J2 > .pin1"]} />
    <trace path={[".U2 > .OUT2", ".J2 > .pin2"]} />
    <trace path={[".U2 > .OUT3", ".J2 > .pin3"]} />
    <trace path={[".U2 > .OUT4", ".J2 > .pin4"]} />
    <trace path={[".J2 > .pin5", "net.VBAT"]} />
  </group>
)
