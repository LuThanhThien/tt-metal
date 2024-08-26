
n = 3
m = 3
k = 3

# Naive matmul
print("Naive matmul")
for mi in range(m):
	for ni in range(n):
		for ki in range(k):
			ai = mi * k + ki
			bi = ki * n + ni
			print(f"Read A[{ai}]; B[{bi}]")
	print("---------------")

print("\n")


# permute
print("Permute")
for ki in range(k):
	for mi in range(m):
		for ni in range(n):
			ai = mi * k + ki
			bi = ki * n + ni
			print(f"Read A[{ai}]; B[{bi}]")
	print("---------------")
print("\n")

