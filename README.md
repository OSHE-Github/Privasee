# Privasee

There are multiple different types of cameras. Detecting with them can be done in a combination of different ways. Some of these way include, but are not limited too detecting the infrared light pulses, reading the Radio Frequency and a handful of other waysPrivasee is a two semester project intended to locate and detect cameras. Additionally, the device itself aims to be low-cost, mobile, and efficient for detection. In other words, the device should be able to scan an area such as a hotel room and indicate to the user if there is a hidden camera. The basis of what to detect for hidden cameras boils down to infrared, radio frequencies, and possibly mac addresses. Infrared is a common trait of hidden cameras since this allows cameras to operate and record in low visibility. Radio frequency is used by hidden cameras to transmit data back to the owner. Mac addresses involve picking up packets sent in the area and displaying the signal strength and address of the device.

<img width="252" height="228" alt="image" src="https://github.com/user-attachments/assets/1a711cd9-5822-4293-a837-f769003cfcc8" />

# Current State
<img width="801" height="538" alt="image" src="https://github.com/user-attachments/assets/a8111454-a73f-4e49-a4fb-f4e32a97b7c9" />

The current prototype includes:
  
  - IR Detection
  - Mac Adress Reading 
  - RF detection (?)

  The system operates as a standalone or works with a laptop to detect cameras

# Project Structure
  
# Features
## Infared Detection
  <img width="400" height="255" alt="image" src="https://github.com/user-attachments/assets/41054f2c-8e92-46ab-bc5c-76b67a3bffbe" />

Many imaging and sensing systems make use of infrared (IR) light for active detection. While standard full-color cameras rely on visible light, some systems emit IR pulses and measure their reflection to detect objects or changes in the environment. Other systems, such as thermal imaging, instead detect existing heat sources and do not require emitted signals.

This circuit focuses on detecting active IR pulses from an external source.

Using the logic shown in the diagram above, the system monitors for the presence of an incoming IR signal and responds when a valid pulse is detected.

The circuit below implements this detection mechanism. An IR sensor (photodiode or phototransistor) converts incoming infrared light into an electrical signal. This signal is then conditioned and amplified so it can be evaluated against a threshold.\

When the detected signal exceeds this threshold:
  - The circuit switches states
  - A control response is triggered
  - A buzzer is activated to indicate detection

This design allows the system to reliably detect IR pulses and convert them into a clear, real-time output signal. When integrated with a larger control loop, it can be used as an input for more complex sensing or decision-making systems.
  
  <img width="576" height="395" alt="image" src="https://github.com/user-attachments/assets/b2910b5c-a2af-43c0-b7f1-1f32abce7de4" />

## Radio Frequency
  Radio Frequency (RF) was chosen due to increasing amount of cameras connecting to Wifi. While connected to WiFi, cameras can capture vidoo and trasmit it. This makes it detectable for as it 
  emits oscilalitng EWF waves that can be picked up. For this semester of the project, the prototype aims to target active cameras transmitting over 2.4GHz.

  <img width="454" height="462" alt="image" src="https://github.com/user-attachments/assets/0c2b6646-c3a5-4613-9b5b-eb8f44e73486" />

### Antenna
  In order to start detecting signal a proper antenna is needed. The antenna will be responsible for picking up wifi signals that devices emit. In this case, the antenna must tuned to a
  resonance frequency of 2.4GHz. With proper tuning, the antenna will pick up these wifi signals more efficiently and this will result in a higher AC voltage.

### Diode Rectification
  Achieving high AC voltage make it much easier to amplify and notice difference in detection. A limiting factor of this AC voltage is that it is operating at 2.4GHz which is much more than
  what the LM358 can support. To amend this, a BAT41 diode was used to help rectify the AC voltage into DC voltage. Advantages of this include readable and processable voltage by the Op amp.
  Downsides of this diode involve voltage drops which results in a weaker signal. A capacitor and resistor is needed in parallel with the rectified diode voltage output in order to “hold” the
  DC voltage so it can be amplified by the OP Amp.

### Amplfication
  As mentioned earlier, amplification is needed to help create a more readable voltage. This it utilizing an LM358 Op amp in a non-inverting configuration. 
  
  <img width="743" height="458" alt="RF Results" src="https://github.com/user-attachments/assets/89dfeb80-50a0-4d19-bc6f-934acf357de4" />

## MAC Address Reading
Many hidden cameras will connect to the internet to upload the videos/pictures they capture. The ESP32's built in WiFi Promiscuous mode can be used to capture packet information and do further analysis on packets in the area. This system only uses information available in IEEE 802.11 packet headers, such as the sending MAC address and data segment length. The ESP32 will also take an RSSI (Received Signal Strength Indicator) measurement when the packet is received. This data helps seperate each packet by a unique device identifier, and enables tracking of the changing signal stregth over time.
The ESP is unable to differentiate between device types by using this information, limiting this features ability to identify hidden cameras. Though this is the case, the MAC addresses and RSSI can be used to located devices in the area that are connected to the internet. 

# Bill of Materials

### RF
|      Item      |  Cost  | Quantity | Total Cost | Source |
| -------------- | ------ | -------- | ---------- | ------ |
| Antenna mount  | $3.86  | 1        | $3.86      | [Link](https://www.digikey.com/en/products/detail/molex/0732511153/11305709) |
| Copper Wire Spool   | ~$7.00 | 1   | ~$7.00     | Lab Resource |
| SMA M TO M Adapter | $6.95 | 1     | $6.95      | [Link](https://www.amazon.com/dp/B0BWJGQC9M) |
| BAT41 Diode    | $0.15  | 1        | $0.30      | [Link](https://www.digikey.com/en/products/detail/stmicroelectronics/BAT41/603518) |
| 1nF Capacitor  | $0.38  | 1        | $0.38      | [Link](https://www.digikey.com/en/products/detail/kemet/C320C102K3G5TA/6656382) |
| 1M Resistor    | $0.10  | 1        | $0.10      | [Link](https://www.digikey.com/en/products/detail/stackpole-electronics-inc/CF14JT1M00/1741316) |
| 10k Resistor   | $0.10  | 1        | $0.10      | [Link](https://www.digikey.com/en/products/detail/stackpole-electronics-inc/CF14JT10K0/1741265) |
| 1k Resistor    | $0.10  | 1        | $0.10      | [Link](https://www.digikey.com/en/products/detail/stackpole-electronics-inc/CF14JT1K00/1741314) |
| Header Pin Pack | $3.29 | 1        | $3.29      | [Link](https://www.digikey.com/en/products/detail/aries-electronics/14-0600-21/49895) |
| Protoboard     | ~$5.00 | 1        | $5.00      | Lab Resource |

# Tools Used

### Software
  Arduino IDE  
  Python  
  KiCad  
  OnShape  
  Git Desktop  

### Hardware
  Solder Iron Station  
  Wire Cutters  
  Wire Crimpers  

# Assembly Instructions

## RF

### Antenna
The SMA mount will be used as basis for the ground plane
Cut two lengths of copper wire to 33mm  
Cut longer than intended length adjustments/tuning can be made afterwords  
Solder one length of the copper wire to the signal pin forming a 90 degree angle  
Solder another length to the SMA mount leg pointing in the opposite direction of the signal pin  

### Board Assembly
Using the schematic for RF assemble the parts on to the protoboard  
Minimize amount of solder used components connecting into V+  
This includes the antenna, BAT41 diode, 1nF capacitor, 1Mohm resistor, and LM358 Op amp  

# Other catagories

### RF Flaws
A big issue with the RF side of this project revolved around a poor testing environment. The testing environment of RF was near an oscilloscope scope which constantly emitted EMF waves. These radiating waves from the oscilloscope cause the antenna on the breadboard to pick up a much stronger signal that what was expected on a 2.4ghz band. This error was caught later on in the semester and resulted in a more rushed prototype of what is currently featured above. While the concept of RF detection is proved, there are underdeveloped areas of RF which include antenna creation and op amp gain. Additionally factors such as parasitic capacitance and type of boards used were not analyzed as well.

The current graph shows some post error data of the current prototype. While this does display a correlation of RF being detected, the gain actual gain is really low. This is mainly due to the BAT41 diode used which did rectify the voltage but resulted in a 0.3V drop. Due to passive antennas not outputting much voltage, there was not a large amount of rectified voltage to be amplified by the op amp. 

# Refrences
[1] “IR cameras - how they work and how to use them,” Sensorland.com, 2026. https://www.sensorland.com/HowPage105.html (accessed Apr. 20, 2026).

‌

