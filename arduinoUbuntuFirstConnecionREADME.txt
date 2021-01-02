//Add dialout if prompted

//list arduino device

ls -l /dev/ttyACM*

The ‘0' at the end of ‘ACM' might be different, and multiple entries might be listed, but the parts we need to focus on are the string of letters and dashes in front, and the two names root and dialout.

The first name root is the owner of the device, and dialout is the owner group of the device.

The letters and dashes in front, starting after ‘c', represent the permissions for the device by user:

    The first triplet rw- mean that the owner (root) can read and write to this device
    The second triplet rw- mean that members of the owner group (dialout) can read and write to this device
    The third triplet --- means that other users have no permissions at all (meaning that nobody else can read and write to the device)

In short, nobody except root and members of dialout can do anything with the Arduino; since we aren't running the IDE as root or as a member of dialout, the IDE can't access the Arduino due to insufficient permissions.

But wait! Earlier, when we were launching the IDE, we did add ourselves to the dialout group!

So why does the IDE still not have permission to access the Arduino?

The changes that the prompt makes don't apply until we log out and log back in again, so we have to save our work, log out, and log back in again.

After you log back in and launch the Arduino IDE, the Serial Port option should be available; change that, and we should be able to upload code to the Arduino.