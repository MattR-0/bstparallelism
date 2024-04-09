import random

num_operations = 10000
operations = []
inserted_keys = set()

# Generate insert operations
for _ in range(int(num_operations * 0.6)):  # 60% inserts
    key = random.randint(1, 10000)
    operations.append(f"insert {key}")
    inserted_keys.add(key)

# Generate search operations
for _ in range(int(num_operations * 0.2)):  # 20% searches
    if random.random() > 0.5 and inserted_keys:
        key = random.choice(list(inserted_keys))  # Search for existing key
    else:
        key = random.randint(1, 10000)  # Search for random key
    operations.append(f"search {key}")

# Generate delete operations
for _ in range(int(num_operations * 0.2)):  # 20% deletes
    if random.random() > 0.5 and inserted_keys:
        key = random.choice(list(inserted_keys))  # Delete existing key
        inserted_keys.remove(key)
    else:
        key = random.randint(1, 10000)  # Delete random key
    operations.append(f"delete {key}")

# Shuffle the operations to mix them up
random.shuffle(operations)

# Write operations to a file
with open("avl_operations.txt", "w") as file:
    for operation in operations:
        file.write(f"{operation}\n")

print("File avl_operations.txt has been created with 10,000 operations.")
