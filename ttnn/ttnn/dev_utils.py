# Description: This file contains utility functions for development purposes.
# Author: Lu Thanh Thien
# First created: 2024-08-21

from pathlib import Path
import ast

ops_dir = Path(__file__).parent.absolute() / "operations"

# unused
def read_operation_file(operation_path):
    count_registered_operations = 0
    with open(operation_path, "r") as f:
        readlines = f.read()
    
    for line in readlines.split("\n"):
        # print(line)
        if line.startswith("@ttnn.register_python_operation"):
            print(line)
            count_registered_operations += 1

    return count_registered_operations

def extract_decorator_names_from_file(file_path, dec_name = "register_python_operation", arg_name = "name"):
    """
    Extracts the names of decorators with the attribute 'register_python_operation' and the keyword argument 'name'
    from a given file.

    Args:
        file_path (str): The path to the file to be parsed.

    Returns:
        list: A list of decorator names.

    """
    with open(file_path, "r") as source_file:
        source_code = source_file.read()

    tree = ast.parse(source_code)
    decorator_names = []

    for node in ast.walk(tree):
        if isinstance(node, ast.FunctionDef):
            for decorator in node.decorator_list:
                if isinstance(decorator, ast.Call) and isinstance(decorator.func, ast.Attribute):
                    if decorator.func.attr == dec_name:
                        for keyword in decorator.keywords:
                            if keyword.arg == "name" and isinstance(keyword.value, ast.Constant):
                                decorator_names.append(keyword.value.s)
    return decorator_names


def get_registered_operations(dec_name = "register_python_operation"):
    registered_operations = []  
    for operation in ops_dir.iterdir():
        if operation.is_file():
            file_path = ops_dir / operation
            # registered_operations.append(file_path)
            registered_operations.extend(extract_decorator_names_from_file(file_path, dec_name))

    return registered_operations


def print_registered_operations():
    """
    Print the registered operations.

    This function retrieves the registered operations and prints them along with the total number of operations.

    Parameters:
        None

    Returns:
        None
    """
    operations = get_registered_operations()
    print("Registered Operations:")
    for operation_name in operations:
        print(f"\t{operation_name}")
    print(f"Number of operations: {len(operations)}")

if __name__ == "__main__":
    print_registered_operations()