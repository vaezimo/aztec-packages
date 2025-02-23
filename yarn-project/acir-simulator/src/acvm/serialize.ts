import { ACVMField, toACVMField } from './acvm.js';

import {
  CallContext,
  ContractDeploymentData,
  FunctionData,
  PrivateCallStackItem,
  PrivateCircuitPublicInputs,
  PublicCallRequest,
} from '@aztec/circuits.js';
import { MessageLoadOracleInputs } from '../client/db_oracle.js';
import { Fr } from '@aztec/foundation/fields';

// Utilities to write TS classes to ACVM Field arrays
// In the order that the ACVM expects them

/**
 * Converts a function data to ACVM fields.
 * @param functionData - The function data to convert.
 * @returns The ACVM fields.
 */
export function toACVMFunctionData(functionData: FunctionData): ACVMField[] {
  return [
    toACVMField(functionData.functionSelector),
    toACVMField(functionData.isPrivate),
    toACVMField(functionData.isConstructor),
  ];
}

/**
 * Converts a call context to ACVM fields.
 * @param callContext - The call context to convert.
 * @returns The ACVM fields.
 */
export function toACVMCallContext(callContext: CallContext): ACVMField[] {
  return [
    toACVMField(callContext.isContractDeployment),
    toACVMField(callContext.isDelegateCall),
    toACVMField(callContext.isStaticCall),
    toACVMField(callContext.msgSender),
    toACVMField(callContext.portalContractAddress),
    toACVMField(callContext.storageContractAddress),
  ];
}

/**
 * Converts a contract deployment data to ACVM fields.
 * @param contractDeploymentData - The contract deployment data to convert.
 * @returns The ACVM fields.
 */
export function toACVMContractDeploymentData(contractDeploymentData: ContractDeploymentData): ACVMField[] {
  return [
    toACVMField(contractDeploymentData.constructorVkHash),
    toACVMField(contractDeploymentData.functionTreeRoot),
    toACVMField(contractDeploymentData.contractAddressSalt),
    toACVMField(contractDeploymentData.portalContractAddress),
  ];
}

/**
 * Converts the public inputs structure to ACVM fields.
 * @param publicInputs - The public inputs to convert.
 * @returns The ACVM fields.
 */
export function toACVMPublicInputs(publicInputs: PrivateCircuitPublicInputs): ACVMField[] {
  return [
    ...toACVMCallContext(publicInputs.callContext),
    toACVMField(publicInputs.argsHash),

    ...publicInputs.returnValues.map(toACVMField),
    ...publicInputs.readRequests.map(toACVMField),
    ...publicInputs.newCommitments.map(toACVMField),
    ...publicInputs.newNullifiers.map(toACVMField),
    ...publicInputs.privateCallStack.map(toACVMField),
    ...publicInputs.publicCallStack.map(toACVMField),
    ...publicInputs.newL2ToL1Msgs.map(toACVMField),
    // TODO #588, relevant issue: https://github.com/AztecProtocol/aztec-packages/issues/588
    // ...publicInputs.encryptedLogsHash.map(toACVMField),
    // ...publicInputs.unencryptedLogsHash.map(toACVMField),

    // toACVMField(publicInputs.encryptedLogPreimagesLength),
    // toACVMField(publicInputs.unencryptedLogPreimagesLength),
    toACVMField(publicInputs.historicPrivateDataTreeRoot),
    toACVMField(publicInputs.historicPrivateNullifierTreeRoot),
    toACVMField(publicInputs.historicContractTreeRoot),
    toACVMField(publicInputs.historicL1ToL2MessagesTreeRoot),

    ...toACVMContractDeploymentData(publicInputs.contractDeploymentData),
  ];
}

/**
 * Converts a private call stack item to ACVM fields.
 * @param item - The private call stack item to convert.
 * @returns The ACVM fields.
 */
export function toAcvmCallPrivateStackItem(item: PrivateCallStackItem): ACVMField[] {
  return [
    toACVMField(item.contractAddress),
    ...toACVMFunctionData(item.functionData),
    ...toACVMPublicInputs(item.publicInputs),
    toACVMField(item.isExecutionRequest),
  ];
}

/**
 * Converts a public call stack item with the request for executing a public function to
 * a set of ACVM fields accepted by the enqueue_public_function_call_oracle Noir function.
 * Note that only the fields related to the request are serialized: those related to the result
 * are empty since this is just an execution request, so we don't send them to the circuit.
 * @param item - The public call stack item to serialize to be passed onto Noir.
 * @returns The fields expected by the enqueue_public_function_call_oracle Noir function.
 */
export async function toAcvmEnqueuePublicFunctionResult(item: PublicCallRequest): Promise<ACVMField[]> {
  return [
    toACVMField(item.contractAddress),
    ...toACVMFunctionData(item.functionData),
    ...toACVMCallContext(item.callContext),
    toACVMField(await item.getArgsHash()),
  ];
}

/**
 * Converts the result of loading messages to ACVM fields.
 * @param messageLoadOracleInputs - The result of loading messages to convert.
 * @param l1ToL2MessagesTreeRoot - The L1 to L2 messages tree root
 * @returns The Message Oracle Fields.
 */
export function toAcvmMessageLoadOracleInputs(
  messageLoadOracleInputs: MessageLoadOracleInputs,
  l1ToL2MessagesTreeRoot: Fr,
): ACVMField[] {
  return [
    ...messageLoadOracleInputs.message.map(f => toACVMField(f)),
    toACVMField(messageLoadOracleInputs.index),
    ...messageLoadOracleInputs.siblingPath.map(f => toACVMField(f)),
    toACVMField(l1ToL2MessagesTreeRoot),
  ];
}

/**
 * Inserts a list of ACVM fields to a witness.
 * @param witnessStartIndex - The index where to start inserting the fields.
 * @param fields - The fields to insert.
 * @returns The witness.
 */
export function toACVMWitness(witnessStartIndex: number, fields: Parameters<typeof toACVMField>[0][]) {
  return fields.reduce((witness, field, index) => {
    witness.set(index + witnessStartIndex, toACVMField(field));
    return witness;
  }, new Map<number, ACVMField>());
}
