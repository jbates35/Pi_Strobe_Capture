# pi_cam_test
For capstone - makes pi strobe light while taking pictures to capture fish eyes

This is to gather data that we can start moving forward with creating algorithms for the fish counter.

For reference, these are the camera settings when we need to use system("...")
![image](https://user-images.githubusercontent.com/70033294/210016663-51e129bb-d8be-4517-9fb9-a2b4929b460f.png)

This program will open a camera stream on your Pi (or ssh'd with x11 forwarding client). It will look like this when first running:
![image](https://user-images.githubusercontent.com/70033294/210021653-9f86d6dd-57d8-4a27-b345-3ac56b5e82ec.png)

Some camera parameters can be controlled:
![image](https://user-images.githubusercontent.com/70033294/210021641-7c1a9ad1-9e58-4a2c-9c9b-98b9361e4e46.png)

When done, it saves to your Pi under this file structure:
![image](https://user-images.githubusercontent.com/70033294/210021442-56f829e6-1cb3-40e7-8fcf-0325a554eafd.png)

When the picture button is pressed (there is a physical button on a breadboard setup this goes with), two images are taken, one with flash and one without flash.
![image](https://user-images.githubusercontent.com/70033294/210021750-a0033efa-39c2-428b-bc70-289ddec4c7f7.png)
![image](https://user-images.githubusercontent.com/70033294/210021773-2309ffd5-f872-4335-b906-911affef1d2f.png)

The video is simply a continual stream of camera shots where the flash is turned on and then off, repeating (i.e. strobed)

There is a log file that goes with each picture or video shot.
![image](https://user-images.githubusercontent.com/70033294/210021814-f5e504d2-c3e1-41d8-80f4-7e0837802275.png)
