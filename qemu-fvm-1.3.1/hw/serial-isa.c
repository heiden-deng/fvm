/*
 * QEMU 16550A UART emulation
 *
 * Copyright (c) 2003-2004 Fabrice Bellard
 * Copyright (c) 2008 Citrix Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "serial.h"
#include "isa.h"

typedef struct ISASerialState {
    ISADevice dev;
    uint32_t index;
    uint32_t iobase;
    uint32_t isairq;
    SerialState state;
} ISASerialState;

static const int isa_serial_io[MAX_SERIAL_PORTS] = {
    0x3f8, 0x2f8, 0x3e8, 0x2e8
};
static const int isa_serial_irq[MAX_SERIAL_PORTS] = {
    4, 3, 4, 3
};

static int serial_isa_initfn(ISADevice *dev)
{
    static int index;
    ISASerialState *isa = DO_UPCAST(ISASerialState, dev, dev);
    SerialState *s = &isa->state;

    if (isa->index == -1) {
        isa->index = index;
    }
    if (isa->index >= MAX_SERIAL_PORTS) {
        return -1;
    }
    if (isa->iobase == -1) {
        isa->iobase = isa_serial_io[isa->index];
    }
    if (isa->isairq == -1) {
        isa->isairq = isa_serial_irq[isa->index];
    }
    index++;

    s->baudbase = 115200;
    isa_init_irq(dev, &s->irq, isa->isairq);
    serial_init_core(s);
    qdev_set_legacy_instance_id(&dev->qdev, isa->iobase, 3);

    memory_region_init_io(&s->io, &serial_io_ops, s, "serial", 8);
    isa_register_ioport(dev, &s->io, isa->iobase);
    return 0;
}

static const VMStateDescription vmstate_isa_serial = {
    .name = "serial",
    .version_id = 3,
    .minimum_version_id = 2,
    .fields = (VMStateField[]) {
        VMSTATE_STRUCT(state, ISASerialState, 0, vmstate_serial, SerialState),
        VMSTATE_END_OF_LIST()
    }
};

static Property serial_isa_properties[] = {
    DEFINE_PROP_UINT32("index",  ISASerialState, index,   -1),
    DEFINE_PROP_HEX32("iobase",  ISASerialState, iobase,  -1),
    DEFINE_PROP_UINT32("irq",    ISASerialState, isairq,  -1),
    DEFINE_PROP_CHR("chardev",   ISASerialState, state.chr),
    DEFINE_PROP_UINT32("wakeup", ISASerialState, state.wakeup, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void serial_isa_class_initfn(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    ISADeviceClass *ic = ISA_DEVICE_CLASS(klass);
    ic->init = serial_isa_initfn;
    dc->vmsd = &vmstate_isa_serial;
    dc->props = serial_isa_properties;
}

static TypeInfo serial_isa_info = {
    .name          = "isa-serial",
    .parent        = TYPE_ISA_DEVICE,
    .instance_size = sizeof(ISASerialState),
    .class_init    = serial_isa_class_initfn,
};

static void serial_register_types(void)
{
    type_register_static(&serial_isa_info);
}

type_init(serial_register_types)

bool serial_isa_init(ISABus *bus, int index, CharDriverState *chr)
{
    ISADevice *dev;

    dev = isa_try_create(bus, "isa-serial");
    if (!dev) {
        return false;
    }
    qdev_prop_set_uint32(&dev->qdev, "index", index);
    qdev_prop_set_chr(&dev->qdev, "chardev", chr);
    if (qdev_init(&dev->qdev) < 0) {
        return false;
    }
    return true;
}
