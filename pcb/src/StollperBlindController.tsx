import { Esp32Module } from "./components/Esp32Module"
import { MotorDriver } from "./components/MotorDriver"
import { PowerManagement } from "./components/PowerManagement"
import { BatteryMonitor } from "./components/BatteryMonitor"
import { MagneticCharger } from "./components/MagneticCharger"
import { ProgrammingHeader } from "./components/ProgrammingHeader"

export const StollperBlindController = () => (
  <board width="44mm" height="58mm" layers={2}>
    <net name="GND" isGroundNet />
    <net name="V3V3" isPowerNet />
    <net name="VBAT" isPowerNet />
    <net name="VBAT_RAW" isPowerNet />
    <net name="CHARGE_IN" isPowerNet />
    <net name="BATT_NEG" />
    <net name="EN" />
    <net name="GPIO0" />
    <net name="TX" />
    <net name="RX" />
    <net name="ADC_BATT" />
    <net name="MOTOR_IN1" />
    <net name="MOTOR_IN2" />
    <net name="MOTOR_IN3" />
    <net name="MOTOR_IN4" />
    <net name="LED_CHRG" />
    <net name="LED_STDBY" />

    {/* GND copper pour on bottom layer — eliminates GND routing congestion */}
    <copperpour
      layer="bottom"
      connectsTo="net.GND"
      boardEdgeMargin="0.5mm"
    />

    {/*
      Layout v4 — 44mm x 58mm
      Copper pour GND bottom → autorouter only handles signal traces
      Test points moved to bottom → no congestion in center

      ┌──────────────────────────────────────┐
      │ MH1         [ANTENNE]          MH2  │  Y=+26
      │          ┌─────────────┐            │
      │  BMON ←  │  ESP32  U1  │  → MOT    │  Y=+15
      │ (gauche) │             │  U2+J2    │
      │          └─────────────┘  (droite) │
      │  C1,C2,R1,R2                       │
      │                                    │
      │  PWR (U3 + U6)                     │  Y=-8
      │  U4,U5,J1    LEDs                 │
      │                                    │
      │  PROG(TP1-6)  J3+D1(charge)       │  Y=-22
      │ MH3                          MH4  │  Y=-26
      └──────────────────────────────────────┘
    */}

    {/* ESP32 — top, antenna at board edge */}
    <Esp32Module name="MCU" pcbX="0mm" pcbY="15mm" />

    {/* Battery monitor — LEFT, near GPIO34 */}
    <BatteryMonitor name="BMON" pcbX="-15mm" pcbY="15mm" />

    {/* Motor driver — RIGHT, near right-side GPIOs */}
    <MotorDriver name="MOT" pcbX="10mm" pcbY="6mm" />

    {/* Power management — lower center */}
    <PowerManagement name="PWR" pcbX="-4mm" pcbY="-8mm" />

    {/* Programming test points — bottom-left (out of the way) */}
    <ProgrammingHeader name="PROG" pcbX="-14mm" pcbY="-22mm" />

    {/* Magnetic charger — bottom-right */}
    <MagneticCharger name="CHG" pcbX="8mm" pcbY="-22mm" />

    {/* Mounting holes — 4 corners */}
    <hole name="MH1" pcbX="-19mm" pcbY="26mm" diameter="3mm" />
    <hole name="MH2" pcbX="19mm" pcbY="26mm" diameter="3mm" />
    <hole name="MH3" pcbX="-19mm" pcbY="-26mm" diameter="3mm" />
    <hole name="MH4" pcbX="19mm" pcbY="-26mm" diameter="3mm" />

    {/* ===== Inter-module traces ===== */}

    {/* RIGHT-side GPIOs → motor driver */}
    <trace path={[".MCU > .U1 > .GPIO23", "net.MOTOR_IN1"]} />
    <trace path={[".MCU > .U1 > .GPIO22", "net.MOTOR_IN2"]} />
    <trace path={[".MCU > .U1 > .GPIO21", "net.MOTOR_IN3"]} />
    <trace path={[".MCU > .U1 > .GPIO19", "net.MOTOR_IN4"]} />

    {/* LEFT-side GPIO34 → battery monitor */}
    <trace path={[".MCU > .U1 > .GPIO34", "net.ADC_BATT"]} />

    {/* UART → programming header */}
    <trace path={[".MCU > .U1 > .GPIO1_TX", "net.TX"]} />
    <trace path={[".MCU > .U1 > .GPIO3_RX", "net.RX"]} />
  </board>
)
