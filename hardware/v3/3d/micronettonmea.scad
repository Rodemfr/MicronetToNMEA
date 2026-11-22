$fs = 0.1;

// Enable parts of the case
mainCaseEnable = true;
topPanelEnable = true;
usbAccessHole = true;

epsilon = 0.001;

// Case
case_thickness = 2;  // Case thickness
case_height = 29;  // Case height
pcb_case_space = 0.2;   // Free space between PCB & case

// PCB
pcbw = 50;  // PCB width
pcbl = 50;  // PCB length
pcbt = 1.6; // PCB thickness

// Board pins
fpx = 3;    // First pin x position from west PCB border
fpy = 3;    // First pin y position from south PCB border
pxd = 39;   // x distance between pins
pxd2 = 30;  // x distance between pins of the second row
pyd = 39;   // y distance between pins

// USB connector
usb_y_pos = 9.75; // y position of the USB connector from south PCB border
usb_width = 8.5;

// Screw terminals
aisty = 6;  // y position from south PCB border (AIS)
sth = 7;    // Height
stw = 16.1; // Width
stfp = 0.5; // Freespace between hole borders and connectors

// Screw pads
spw = 10;   // Pad width
sdia = 3.2; // screw diameter

// RF Antenna
antw = 7;  // Hole diameter
anty = 41; // y position of the antenna hole from south PCB border
antz = 14; // z position of the antenna hole from PCB surface

// Board pin definition
ph = 4;     // Height
pr = 2;     // Radius
pshr = 0.9; // Screw hole radius
module boardPin(x, y)
{
    difference()
    { // Pin
        translate([ x, y, 0 ]) cylinder(ph + case_thickness, pr, pr);
        // Screw hole
        translate([ x, y, 0 ]) cylinder(ph + case_thickness + 0.1, pshr, pshr);
    }
}

// Screw pad definition
module screwPad()
{
    difference()
    {
        cube([ spw, spw, case_thickness + 1 ]);
        translate([ spw / 2, spw / 2, -1 ]) cylinder(case_thickness + 3, sdia / 2 + 0.1, sdia / 2 + 0.1);
        translate([ spw / 2, spw / 2, 1.5 + 0.01 ]) cylinder(1.5, sdia / 2 + 0.1, sdia / 2 + 1.8 + 0.1);
    }
}

if (mainCaseEnable == true)
{
    // Bottom panel
    cube([ pcbw + case_thickness * 2 + pcb_case_space * 2, pcbl + case_thickness * 2 + pcb_case_space * 2, case_thickness ]);

    // Board pins
    boardPin(case_thickness + pcb_case_space + 3, case_thickness + pcb_case_space + 3);
    boardPin(case_thickness + pcb_case_space + pcbw - 3, case_thickness + pcb_case_space + 3);
    boardPin(case_thickness + pcb_case_space + 14, case_thickness + pcb_case_space + pcbl - 3);

    cube([ pcbw + case_thickness * 2 + pcb_case_space * 2, case_thickness, pcbt + ph + case_thickness], center = false);
    translate([0, pcbl + case_thickness + pcb_case_space * 2, 0])
        cube([ pcbw + case_thickness * 2 + pcb_case_space * 2, case_thickness, pcbt + ph + case_thickness], center = false);
    cube([ case_thickness, pcbl + case_thickness * 2 + pcb_case_space * 2, pcbt + ph + case_thickness], center = false);
    translate([pcbw + case_thickness * 1 + pcb_case_space * 2, 0, 0]) {
        difference() {
            cube([ case_thickness, pcbl + case_thickness * 2 + pcb_case_space * 2, pcbt + ph + case_thickness], center = false);
            translate([-epsilon, usb_y_pos - usb_width / 2 + case_thickness + pcb_case_space, pcbt + ph - 1 + case_thickness])
                cube([ case_thickness + 2 * epsilon, usb_width, 1 + epsilon], center = false);
        }
    };
};