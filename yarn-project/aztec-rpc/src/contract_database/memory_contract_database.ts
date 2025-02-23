import { AztecAddress } from '@aztec/foundation/aztec-address';
import { ContractDao } from './contract_dao.js';
import { ContractDatabase } from './contract_database.js';

/**
 * The MemoryContractDatabase class serves as an in-memory implementation of the ContractDatabase interface.
 * It allows for storing and retrieving contract data, such as ContractDao objects and associated function bytecodes,
 * within a contracts array. This class is particularly useful for testing and development purposes where a
 * persistent storage may not be required.
 */
export class MemoryContractDatabase implements ContractDatabase {
  private contracts: ContractDao[] = [];

  /**
   * Adds a new ContractDao instance to the memory-based contract database.
   * The function stores the contract in an array and returns a resolved promise indicating successful addition.
   *
   * @param contract - The ContractDao instance to be added to the memory database.
   * @returns A Promise that resolves when the contract is successfully added.
   */
  public addContract(contract: ContractDao) {
    this.contracts.push(contract);
    return Promise.resolve();
  }

  /**
   * Retrieve a ContractDao instance with the specified AztecAddress from the in-memory contracts list.
   * Returns the first match found or undefined if no contract with the given address is found.
   *
   * @param address - The AztecAddress to search for in the stored contracts.
   * @returns A Promise resolving to the ContractDao instance matching the given address or undefined.
   */
  public getContract(address: AztecAddress) {
    return Promise.resolve(this.contracts.find(c => c.address.equals(address)));
  }

  /**
   * Retrieve the bytecode associated with a given contract address and function selector.
   * This function searches through the stored contracts to find a matching contract and function,
   * then returns the corresponding bytecode. If no match is found, it returns undefined.
   *
   * @param contractAddress - The AztecAddress representing the contract address to look for.
   * @param functionSelector - The Buffer containing the function selector to search for.
   * @returns A Promise that resolves to the bytecode of the matching function or undefined if not found.
   */
  public async getCode(contractAddress: AztecAddress, functionSelector: Buffer) {
    const contract = await this.getContract(contractAddress);
    return contract?.functions.find(f => f.selector.equals(functionSelector))?.bytecode;
  }
}
