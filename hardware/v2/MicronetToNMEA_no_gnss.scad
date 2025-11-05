$fs = 0.1;

// Enable parts of the case
mainCaseEnable = true;
topPanelEnable = true;
usbAccessHole = true;

// Case
ctn = 2;  // Case thickness
ch = 29;  // Case height
fs = 1;   // Free space between PCB & case
efs = 4;  // Extra free space on north & west borders to allow inserting PCB in case

// PCB
pcbw = 45;  // PCB width
pcbl = 45;  // PCB length
pcbt = 1.6; // PCB thickness

// Board pins
ph = 4;     // Height
pr = 2;     // Radius
pshr = 0.9; // Screw hole radius
fpx = 3;    // First pin x position from west PCB border
fpy = 3;    // First pin y position from south PCB border
pxd = 39;   // x distance between pins
pxd2 = 30;  // x distance between pins of the second row
pyd = 39;   // y distance between pins

// Screw terminals
aisty = 6;  // y position from south PCB border (AIS)
sth = 7;    // Height
stw = 16.1; // Width
stfp = 0.5; // Freespace between hole borders and connectors

// Screw pads
spw = 10;   // Pad width
sdia = 3.2; // screw diameter

// Teensy USB connector
tuch = 8;
tucw = 13;
tucx = 16;
tucz = 14;

// RF Antenna
antw = 7;  // Hole diameter
anty = 41; // y position of the antenna hole from south PCB border
antz = 14; // z position of the antenna hole from PCB surface

// Board pin definition
module boardPin(x, y)
{
    difference()
    { // Pin
        translate([ x, y, 0 ]) cylinder(ph + ctn, pr, pr);
        // Screw hole
        translate([ x, y, 0 ]) cylinder(ph + ctn + 0.1, pshr, pshr);
    }
}

// Screw pad definition
module screwPad()
{
    difference()
    {
        cube([ spw, spw, ctn + 1 ]);
        translate([ spw / 2, spw / 2, -1 ]) cylinder(ctn + 3, sdia / 2 + 0.1, sdia / 2 + 0.1);
        translate([ spw / 2, spw / 2, 1.5 + 0.01 ]) cylinder(1.5, sdia / 2 + 0.1, sdia / 2 + 1.8 + 0.1);
    }
}

if (mainCaseEnable == true)
{
    // Bottom panel
    cube([ pcbw + ctn * 2 + fs * 2 + efs, pcbl + ctn * 2 + fs * 2, ctn ]);

    // South panel
    difference()
    {
        // Panel
        cube([ pcbw + ctn * 2 + fs * 2 + efs, ctn, ch ]);
        if (usbAccessHole == true)
        {
            // Teensy USB connector hole
            translate([ tucx + ctn + fs + efs / 2, -0.1, ctn + ph + pcbt + tucz ]) cube([ tucw, ctn + 3, tuch ], center = true);
        };
    }

    // North panel
    translate([ 0, pcbl + ctn * 2 + fs * 2 - ctn, 0 ]) cube([ pcbw + ctn * 2 + fs * 2 + efs, ctn, ch ]);

    // West panel
    difference()
    {
        cube([ ctn, pcbl + ctn * 2 + fs * 2, ch ]);
                // 868MHz Antenna hole
        translate([ -5 - ctn - 0.1 - efs, anty + fs, ctn + ph + pcbt + antz ]) rotate([ 0, 90, 0 ])
            cylinder(15, antw / 2, antw / 2);
    }

    // East panel
    difference()
    {
        // Panel
        translate([ pcbw + ctn * 2 + fs * 2 - ctn + efs, 0, 0 ]) cube([ ctn, pcbl + ctn * 2 + fs * 2, ch ]);
        // Power/AIS connector hole
        translate([ pcbw + ctn * 2 + fs * 2 - ctn - 0.1 + efs, aisty + ctn + fs - stfp, ctn + ph + pcbt - stfp ])
            cube([ 5, stw + stfp * 2, sth + stfp * 2 ]);
    };

    // PCB pins
    boardPin(fpx + fs + efs, fpy + ctn + fs);
    boardPin(fpx + fs + efs, fpy + ctn + fs + pyd);
    boardPin(fpx + pxd2 + fs + efs, fpy + ctn + fs + pyd);
    boardPin(fpx + fs + pxd + efs, fpy + ctn + fs);

    // Screw pads
    translate([ -spw + 0.001, pcbl / 2 + spw / 4 + fs * 2 - spw + efs, 0 ]) screwPad();
    translate([ pcbw + ctn * 2 + fs * 2 + efs - 0.001, pcbl / 2 + spw / 4 + fs * 2 - spw + efs, 0 ]) screwPad();
}

// Top panel
if (topPanelEnable == true)
{
    panelShift = 20;

    translate([ pcbw + fs * 2 + panelShift, 0, 0 ]) cube([ pcbw + ctn * 2 + fs * 2 + efs, pcbl + ctn * 2 + fs * 2, ctn ]);
    translate([ pcbw + fs * 2 + panelShift + ctn + 0.1, ctn + 0.1, ctn ])
        difference()
        {
            cube([ pcbw + ctn * 2 + fs * 2 + efs - ctn * 2 - 0.2, pcbl + ctn * 2 + fs * 2 - ctn * 2 - 0.2, 2.1 ]);
            translate([ 1.5, 1.5, -0.1 ])
                cube([ pcbw + fs * 2 + efs - 0.2 - 3, pcbl + fs * 2 - 0.2 - 3, 2.5 ]);
        }
}

if (usbAccessHole == true)
{
    usbPlugShift = pcbl + fs * 2 + 5;
    usbPlugExt = 1.5;
    usbPlugMargin = 0.2;

    translate([ usbPlugExt, usbPlugShift + usbPlugExt + usbPlugMargin, 1 ]) cube([ tucw, tuch - usbPlugMargin * 2, ctn]);
    translate([ 0, usbPlugShift, 0 ]) cube([ tucw + usbPlugExt * 2, tuch + usbPlugExt * 2, 1]);
};