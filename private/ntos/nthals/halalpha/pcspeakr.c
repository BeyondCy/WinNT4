/*++

Copyright (c) 1992, 1993  Digital Equipment Corporation

Module Name:

    pcspeakr.c

Abstract:

    This module implements the HAL speaker "beep" routines for the
    Alpha system that use the standard PC speaker driven by an interval
    timer.

Author:

    Jeff McLeman (mcleman)  23-Jun-1992

Environment:

    Kernel mode

Revision History:

--*/

#include "halp.h"
#include "eisa.h"


BOOLEAN
HalMakeBeep(
    IN ULONG Frequency
    )

/*++

Routine Description:

    This function sets the frequency of the speaker, causing it to sound a
    tone.  The tone will sound until the speaker is explicitly turned off,
    so the driver is responsible for controlling the duration of the tone.

Arguments:

    Frequency - Supplies the frequency of the desired tone.  A frequency of
                0 means the speaker should be shut off.

Return Value:

    TRUE  - Operation was successful (frequency within range or zero).
    FALSE - Operation was unsuccessful (frequency was out of range).
            Current tone (if any) is unchanged.

--*/

{
    KIRQL oldIrql;
    NMI_STATUS NmiStatus;
    PEISA_CONTROL controlBase;
    TIMER_CONTROL timerControl;
    ULONG newCount;

    controlBase = HalpEisaControlBase;

    //
    // Raise the IRQL to dispatch level and acquire the beep spinlock.
    //

    KeAcquireSpinLock(&HalpBeepLock, &oldIrql);

    //
    // Stop the speaker.
    //

    *((PUCHAR) &NmiStatus) = READ_PORT_UCHAR(&controlBase->NmiStatus);

    NmiStatus.SpeakerGate = 0;
    NmiStatus.SpeakerData = 0;

    WRITE_PORT_UCHAR(&controlBase->NmiStatus, *((PUCHAR) &NmiStatus));

    if (Frequency == 0) {
        KeReleaseSpinLock(&HalpBeepLock, oldIrql);
        return(TRUE);
    }

    //
    // Calculate the new counter value.
    //

    newCount = TIMER_CLOCK_IN / Frequency;

    //
    // The new count must be less than 16 bits in value.
    //
    
    if (newCount >= 0x10000) {
        KeReleaseSpinLock(&HalpBeepLock, oldIrql);
        return(FALSE);
    }

    //
    // Set the speaker timer to the correct mode.
    //

    timerControl.BcdMode = 0;
    timerControl.Mode = TM_SQUARE_WAVE;
    timerControl.SelectByte = SB_LSB_THEN_MSB;
    timerControl.SelectCounter = SELECT_COUNTER_2;

    WRITE_PORT_UCHAR(&controlBase->CommandMode1, *((PUCHAR) &timerControl));

    //
    // Set the speaker timer to the correct mode.
    //

    WRITE_PORT_UCHAR(&controlBase->SpeakerTone, (UCHAR)newCount);
    WRITE_PORT_UCHAR(&controlBase->SpeakerTone, (UCHAR)(newCount >> 8));

    //
    // Start the speaker.
    //

    NmiStatus.SpeakerGate = 1;
    NmiStatus.SpeakerData = 1;

    WRITE_PORT_UCHAR(&controlBase->NmiStatus, *((PUCHAR) &NmiStatus));

    //
    // Release the beep spinlock and restore the IRQL.
    //

    KeReleaseSpinLock(&HalpBeepLock, oldIrql);
    return(TRUE);
}
