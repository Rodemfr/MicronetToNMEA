$fs = 0.1;

// Enable parts of the case
topPanelEnable = 1;
mainCaseEnable = 1;
usbAccessHole = 1;

// Case
ctn = 2;  // Case thickness
ch = 30;  // Case height
fs = 3.5; // Free space between PCB & case
efs = 2;  // Extra free space on north & west borders to allow inserting PCB in case

// PCB
pcbw = 51.539; // PCB width
pcbl = 56.644; // PCB length
pcbt = 1.6;    // PCB thickness

// Board pins
ph = 5;       // Height
pr = 2;       // Radius
pshr = 0.9;   // Screw hole radius
fpx = 3.048;  // First pin x position from west PCB border
fpy = 2.921;  // First pin y position from south PCB border
pxd = 45.720; // x distance between pins
pyd = 34.290; // y distance between pins

// Screw terminals
pstx = 29.969;  // x position from west PCB border (+12V)
aisty = 17.497; // y position from south PCB border (AIS)
sth = 8;        // Height
stw = 9;        // Width
stfp = 0.5;     // Freespace between hole borders andconnectors

// Screw pads
spw = 10; // Pad width
sdia = 3; // screw diameter

// Teensy USB connector
tuch = 10;
tucw = 13;
tucx = 11.808 + 15.24 / 2;
tucz = 14.5;

// RF Antenna
antw = 7;  // Hole diameter
anty = 45; // y position of the antenna hole from south PCB border
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
        translate([ spw / 2, spw / 2, 1.5 + 0.01 ]) cylinder(1.5, sdia / 2 + 0.1, sdia / 2 + 2 + 0.1);
    }
}

if (mainCaseEnable == 1)
{
    // Bottom panel
    cube([ pcbw + fs * 2 + efs, pcbl + fs * 2 + efs, ctn ]);

    // South panel
    difference()
    {
        // Panel
        cube([ pcbw + fs * 2 + efs, ctn, ch ]);
        // Power supply connector hole
        translate([ pstx + fs - stfp + efs, -0.1, ctn + ph + pcbt - stfp ])
            cube([ stw + stfp * 2, ctn + 1, sth + stfp * 2 ]);
        if (usbAccessHole == 1)
        {
            // Teensy USB connector hole
            translate([ tucx + fs + efs, -0.1, ctn + ph + pcbt + tucz ]) cube([ tucw, ctn + 3, tuch ], center = true);
        };
    }

    // North panel
    translate([ 0, pcbl + fs * 2 - ctn + efs, 0 ]) cube([ pcbw + fs * 2 + efs, ctn, ch ]);

    // West panel
    cube([ ctn, pcbl + fs * 2 + efs, ch ]);

    // East panel
    difference()
    {
        // Panel
        translate([ pcbw + fs * 2 - ctn + efs, 0, 0 ]) cube([ ctn, pcbl + fs * 2 + efs, ch ]);
        // AIS connector hole
        translate([ pcbw + fs * 2 - ctn - 0.1 + efs, aisty + fs - stfp, ctn + ph + pcbt - stfp ])
            cube([ 5, stw + stfp * 2, sth + stfp * 2 ]);
        // 868MHz Antenna hole
        translate([ pcbw + fs * 2 - ctn - 0.1 + efs, anty + fs, ctn + ph + pcbt + antz ]) rotate([ 0, 90, 0 ])
            cylinder(10, antw / 2, antw / 2);
    };

    // PCB pins
    boardPin(fpx + fs + efs, fpy + fs);
    boardPin(fpx + fs + efs, fpy + fs + pyd);
    boardPin(fpx + fs + pxd + efs, fpy + fs + pyd);

    // HC-06 pin
    translate([ fs + 15 + efs, fs + pcbl - 9, 0 ]) cylinder(2 + ctn, pr, pr);

    // Screw pads
    translate([ -spw + 0.001, 0, 0 ]) screwPad();
    translate([ -spw + 0.001, pcbl + fs * 2 - spw + efs, 0 ]) screwPad();
    translate([ pcbw + fs * 2 + efs - 0.001, 0, 0 ]) screwPad();
    translate([ pcbw + fs * 2 + efs - 0.001, pcbl + fs * 2 - spw + efs, 0 ]) screwPad();
}

// Top panel
if (topPanelEnable == 1)
{
    panelShift = 15;

    difference()
    {
        translate([ pcbw + fs * 2 + panelShift, 0, 0 ]) cube([ pcbw + fs * 2 + efs, pcbl + fs * 2 + efs, ctn ]);
        // MicronetToNMEA Logo
        translate([ pcbw + fs * 2 + panelShift + ctn + 56, pcbl * 0.60 + ctn, -0.5 ]) mirror(v = [ 1, 0, 0 ])
            linear_extrude(height = 1) text("MicronetToNMEA", size = 4.2, font = "Sans:style=Bold");
    }
    translate([ pcbw + fs * 2 + panelShift + ctn + 0.1, ctn + 0.1, ctn ]) difference()
    {
        cube([ pcbw + fs * 2 + efs - ctn * 2 - 0.2, pcbl + fs * 2 + efs - ctn * 2 - 0.2, 2.1 ]);
        translate([ 1.5, 1.5, -0.1 ])
            cube([ pcbw + fs * 2 + efs - ctn * 2 - 0.2 - 3, pcbl + fs * 2 + efs - ctn * 2 - 0.2 - 3, 2.5 ]);
    }
}

if (usbAccessHole == 1)
{
    usbPlugShift = pcbl + fs * 2 + 15;
    usbPlugExt = 1.5;
    usbPlugMargin = 0.2;

    translate([ usbPlugExt, usbPlugShift + usbPlugExt + usbPlugMargin, 1 ]) cube([ tucw, tuch - usbPlugMargin * 2, ctn]);
    translate([ 0, usbPlugShift, 0 ]) cube([ tucw + usbPlugExt * 2, tuch + usbPlugExt * 2, 1]);
};