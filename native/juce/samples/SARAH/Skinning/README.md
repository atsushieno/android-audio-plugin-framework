# Skinning resources for SARAH #

This folder contains some resources in support of a simplistic approach to "skinning" the SARAH plugin.

As of 29 September, 2017, SARAH uses a single-view GUI instead of the original multi-tabbed GUI approach inherited from [VanillaJuce](https://github.com/getdunne/VanillaJuce). The tabbed approach is useful in development of plugins, while you are still figuring out what parameters you need. Once the parameter set is stable, however, the single-view approach is arguably better. (See http://getdunne.net/wiki/doku.php?id=sarah_skinning for some discussion.)

The present version of SARAH uses a PNG image as the background for the single GUI page. The default background image (aka "skin") is the file *Resources/background.png*, which gets compiled into the plugin using the JUCE *BinaryData* mechanism. Each time the plugin is instantiated, it loads this default skin image first, then looks in the user's *Desktop* folder for a file *sarah.png*. If it finds one, it loads that image instead.

**Update Oct 1, 2017** Although the code still contains support for a default background image, it will only be used if you change the default value of *useBackgroundImage* to *true* in *PluginEditor.cpp*. By default, the program creates a pure vector-graphics GUI, which should work on any computer (and based on what I have read, I **think** it should scale automatically on Retina-screen Macs, but I don't have one to verify this).

This folder contains some resources for creating SARAH skin images using [Paint.NET](https://www.getpaint.net/), a free image-editing program which supports multiple *layers*.

**SarahSkin.pdn** is the basic Paint.NET file. To create the default skin:
1. Open *SarahSkin.pdn* in Paint.NET
2. Choose *Save As* from the *File* menu, specify file type "png", navigate to the *Resources* folder and specify the name "background.png".
    - Confirm that it's OK to overwrite the existing file
3. Accept the defaults in the "Save Configuration" dialog which appears
4. Click on "Flatten" in the "Save" dialog, telling the program to merge the layers
	- It's OK to overwrite the existing file
	- You want to "flatten" the image first, by merging the layers
5. Close Paint.NET
    - After the "Save As" operation, Paint.NET will have *Resources/background.png* open, **not** the original *SarahSkin.pdn* file.

The *SarahSkin.pdn* document contains three layers:
1. The *Background* layer is just a solid RGB colour (red 50, green 62, blue 68, which is the default background colour used in the original GUI code generated by the JUCE Projucer).
2. The *GroupsAndLabels* layer is a transparent overlay containing SARAH's group boxes and the labels for all the knobs.
3. The *Sarah* layer is another transparent overlay containing just the SARAH logo.

By "transparent overlay", I mean an RGBA (RGB plus alpha) image where the Alpha component is zero for all transparent pixels, 255 for all fully-opaque pixels, and something in between for "anti-aliased" pixels at the edges of the drawn structures. Such overlay images, or *masks* as they are sometimes called, can be a bit difficult to create without professional software such as *Adobe Photoshop*. I created them out of ordinary "opaque" image files, using a couple of simple Python programs which are also included here:
- *MakeGroupsAndLabelsMask.py* processes the opaque image file *GroupsAndLabels.png* to create *GroupsAndLabelsMask.py*
- *MakeSarahMask.py* processes *Sarah.png* to create *SarahMask.png*

To run these programs, you'll need [Python](https://www.python.org/) plus the [PyPNG library](https://github.com/drj11/pypng) (The command *pip install pypng* will install this automatically on most Python setups.)

After you create the RGBA mask files, you can drag them into the Paint.NET window, and Paint.NET will offer you the option of adding the mask image as a new layer to the presently-open file.

The SARAH source-code file *PluginEditor.cpp* includes code to create and position the group boxes and label text as well as all the other controls, and I have included a few *#define*s at the top, to allow you to determine which of these actually appear. To create the *GroupsAndLabels.png* image, I set these to show the group boxes and labels, but suppress display of the actual controls, then compiled and ran the plugin and took a screen shot. Then I went back in and set the *#define*s the other way, to show only the controls. This approach gives you options to include or not include labels and group outlines in your skin files.

I'm well aware of the limitations of this approach to skinning. See http://getdunne.net/wiki/doku.php?id=sarah_skinning for some further discussion.