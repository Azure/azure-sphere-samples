/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef MT3620_BAREMETAL_H
#define MT3620_BAREMETAL_H

#include <stdint.h>
#include <stddef.h>

/// <summary>Base address of System Control Block, ARM DDI 0403E.b S3.2.2.</summary>
static const uintptr_t SCB_BASE = 0xE000ED00;
/// <summary>Base address of NVIC Set-Enable Registers, ARM DDI 0403E.b S3.4.3.</summary>
static const uintptr_t NVIC_ISER_BASE = 0xE000E100;
/// <summary>Base address of NVIC Interrupt Priority Registers, ARM DDI 0403E.b S3.4.3.</summary>
static const uintptr_t NVIC_IPR_BASE = 0xE000E400;

/// <summary>The IOM4 cores on the MT3620 use three bits to encode interrupt priorities.</summary>
#define IRQ_PRIORITY_BITS 3

/// <summary>
/// Zero-argument callback.
/// </summary>
typedef void (*Callback)(void);

/// <summary>
/// Write the supplied 8-bit value to an address formed from the supplied base
/// address and offset.
/// </summary>
/// <param name="baseAddr">Typically the start of a register bank.</param>
/// <param name="offset">This value is added to the base address to form the target address.
/// It is typically the offset of a register within a bank.</param>
/// <param name="value">8-bit value to write to the target address.</param>
static inline void WriteReg8(uintptr_t baseAddr, size_t offset, uint8_t value)
{
    *(volatile uint8_t *)(baseAddr + offset) = value;
}

/// <summary>
/// Write the supplied 32-bit value to an address formed from the supplied base
/// address and offset.
/// </summary>
/// <param name="baseAddr">Typically the start of a register bank.</param>
/// <param name="offset">This value is added to the base address to form the target address.
/// It is typically the offset of a register within a bank.</param>
/// <param name="value">32-bit value to write to the target address.</param>
static inline void WriteReg32(uintptr_t baseAddr, size_t offset, uint32_t value)
{
    *(volatile uint32_t *)(baseAddr + offset) = value;
}

/// <summary>
/// Read a 32-bit value from an address formed from the supplied base
/// address and offset.
/// </summary>
/// <param name="baseAddr">Typically the start of a register bank.</param>
/// <param name="offset">This value is added to the base address to form the target address.
/// It is typically the offset of a register within a bank.</param>
/// <returns>An unsigned 32-bit value which is read from the target address.</returns>
static inline uint32_t ReadReg32(uintptr_t baseAddr, size_t offset)
{
    return *(volatile uint32_t *)(baseAddr + offset);
}

/// <summary>
/// <para>Read a 32-bit register from the supplied address, clear the supplied bits,
/// and write the new value back to the register.</para>
/// <para>This is not an atomic operation. If the value of the register is liable
/// to change between the read and write operations, the caller should use
/// appropriate locking.</para>
/// </summary>
/// <param name="baseAddr">Typically the start of a register bank.</param>
/// <param name="offset">This value is added to the base address to form the target address.
/// It is typically the offset of a register within a bank.</param>
/// <param name="clearBits">Bits which should be cleared in the final value.</param>
static inline void ClearReg32(uintptr_t baseAddr, size_t offset, uint32_t clearBits)
{
    uint32_t value = ReadReg32(baseAddr, offset);
    value &= ~clearBits;
    WriteReg32(baseAddr, offset, value);
}

/// <summary>
/// <para>Read a 32-bit register from the supplied address, set the supplied bits,
/// and write the new value back to the register.</para>
/// <para>This is not an atomic operation. If the value of the register is liable
/// to change between the read and write operations, the caller should use
/// appropriate locking.</para>
/// </summary>
/// <param name="baseAddr">Typically the start of a register bank.</param>
/// <param name="offset">This value is added to the base address to form the target address.
/// It is typically the offset of a register within a bank.</param>
/// <param name="setBits">Bits which should be cleared in the final value.</param>
static inline void SetReg32(uintptr_t baseAddr, size_t offset, uint32_t setBits)
{
    uint32_t value = ReadReg32(baseAddr, offset);
    value |= setBits;
    WriteReg32(baseAddr, offset, value);
}

/// <summary>
/// <para>Blocks interrupts at priority 1 level and above.</para>
/// <para>Pair this with a call to <see cref="RestoreIrqs" /> to unblock interrupts.</para>
/// </summary>
/// <returns>Previous value of BASEPRI register. This can be treated as an opaque value
/// which must be passed to <see cref="RestoreIrqs" />.</returns>
static inline uint32_t BlockIrqs(void)
{
    uint32_t prevBasePri;
    uint32_t newBasePri = 1; // block IRQs priority 1 and above

    __asm__("mrs %0, BASEPRI" : "=r"(prevBasePri) :);
    __asm__("msr BASEPRI, %0" : : "r"(newBasePri));
    return prevBasePri;
}

/// <summary>
/// Re-enables interrupts which were blocked by <see cref="BlockIrqs" />.
/// </summary>
/// <param name="prevBasePri">Value returned from <see cref="BlockIrqs" />.</param>
static inline void RestoreIrqs(uint32_t prevBasePri)
{
    __asm__("msr BASEPRI, %0" : : "r"(prevBasePri));
}

/// <summary>
/// <para>Set NVIC priority for the supplied interrupt.</para>
/// <para>See ARM DDI 0403E.d SB3.4.9, Interrupt Priority Registers, NVIC_IPR0-NVIC_IPR123.</para>
/// <para><see cref="EnableNvicInterrupt" /></para>
/// </summary>
/// <param name="irqNum">Which interrupt to set the priority for.</param>
/// <param name="pri">Priority, which must fit into the number of supported priority bits.</param>
static inline void SetNvicPriority(int irqNum, uint8_t pri)
{
    WriteReg8(NVIC_IPR_BASE, irqNum, pri << ((8 - IRQ_PRIORITY_BITS)));
}

/// <summary>
/// <para>Enable NVIC interrupt.</para>
/// <para>See DDI 0403E.d SB3.4.4, Interrupt Set-Enable Registers, NVIC_ISER0-NVIC_ISER15.</para>
/// <para><see cref="SetNvicPriority" /></para>
/// </summary>
/// <param name="irqNum">Which interrupt to enable.</param>
static inline void EnableNvicInterrupt(int irqNum)
{
    size_t offset = 4 * (irqNum / 32);
    uint32_t mask = 1U << (irqNum % 32);
    SetReg32(NVIC_ISER_BASE, offset, mask);
}

#endif /* MT3620_BAREMETAL_H */
