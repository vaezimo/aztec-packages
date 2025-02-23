import {
  CircuitError,
  KERNEL_PUBLIC_DATA_READS_LENGTH,
  makeTuple,
  simulatePublicKernelCircuit,
  simulatePublicKernelCircuitNoPreviousKernel,
} from '../index.js';
import {
  makePublicDataRead,
  makePublicKernelInputsNoKernelInput,
  makePublicKernelInputsWithEmptyOutput,
} from '../tests/factories.js';

describe('kernel/public_kernel', () => {
  it('simulates public kernel circuit with previous public kernel', async function () {
    const input = await makePublicKernelInputsWithEmptyOutput();
    // Fix validity
    input.publicCall.callStackItem.functionData.isConstructor = false;
    input.publicCall.callStackItem.functionData.isPrivate = false;
    input.previousKernel.publicInputs.isPrivate = false;
    const result = await simulatePublicKernelCircuit(input);
    expect(result).toBeDefined();
  });

  it('simulates public kernel circuit with previous private kernel', async function () {
    const input = await makePublicKernelInputsWithEmptyOutput();
    input.previousKernel.publicInputs.isPrivate = true;
    const result = await simulatePublicKernelCircuit(input);
    expect(result).toBeDefined();
  });

  it('simulates public kernel circuit with no previous kernel', async function () {
    const input = await makePublicKernelInputsNoKernelInput();
    const result = await simulatePublicKernelCircuitNoPreviousKernel(input);
    expect(result).toBeDefined();
  });

  it('simulating public kernel circuit fails when aggregating proofs will overflow', async function () {
    const input = await makePublicKernelInputsWithEmptyOutput();
    // Fix validity
    input.publicCall.callStackItem.functionData.isConstructor = false;
    input.publicCall.callStackItem.functionData.isPrivate = false;
    input.previousKernel.publicInputs.isPrivate = false;

    // Cause array overflow
    const fullStateReadsObject = makeTuple(KERNEL_PUBLIC_DATA_READS_LENGTH, makePublicDataRead, 0x01);
    input.previousKernel.publicInputs.end.publicDataReads = fullStateReadsObject;

    await expect(simulatePublicKernelCircuit(input)).rejects.toThrow(
      new CircuitError(7009, 'array_push cannot push to a full array'),
    );
  });
});
