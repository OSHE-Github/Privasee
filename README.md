# Privasee

There are multiple different types of cameras. Detecting with them can be done in a combination of different ways. Some of these way include, but are not limited too detecting the infrared light pulses, reading the Radio Frequency and a handful of other waysPrivasee is a two semester project intended to locate and detect cameras. Additionally, the device itself aims to be low-cost, mobile, and efficient for detection. In other words, the device should be able to scan an area such as a hotel room and indicate to the user if there is a hidden camera. The basis of what to detect for hidden cameras boils down to infrared, radio frequencies, and possibly mac addresses. Infrared is a common trait of hidden cameras since this allows cameras to operate and record in low visibility. Radio frequency is used by hidden cameras to transmit data back to the owner. Mac addresses involve picking up packets sent in the area and displaying the signal strength and address of the device.

<img width="252" height="228" alt="image" src="https://github.com/user-attachments/assets/1a711cd9-5822-4293-a837-f769003cfcc8" />

# Current State

The current prototype includes:
  
  *IR Detection
  
  *Mac Adress Reading 
  
  *RF detection (?)

  The system operates as a standalone or works with a laptop to detect cameras

# Project Structure
  
# Features
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

# Other catagories
