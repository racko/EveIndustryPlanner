import numpy as np

#class Resource:
#    def __init__(self, available):
#        self.available = available
#
#class Job:
#    def __init__(self, inputs, output, cost, limit):
#        self.inputs = inputs
#        self.outputs = outputs
#        self.cost = cost
#        self.limit = limit

def main():
m = 29 # Jobs
n = 10 # Resources
A = np.zeros((n,m))
A[:,:9] = np.random.randint(1,10,size=(n,9)) # 10: max consumed / produced
A[:,9:19] = -np.eye(n)
A[:,19:] = np.eye(n)
produced = np.arange(1,n)
A[produced,np.arange(9)] = -A[produced,np.arange(9)]
for i in xrange(9):
    A[produced[i]+1:,i] = 0
c = np.abs(np.random.randn(m))
c[:9] = -c[:9]
c[9:19] = -100.0 * c[9:19]
c[19:] = 100.0 * c[19:]
b = np.random.randint(1,10,size=n) # 10: max already available
job_limits = np.random.randint(1,10,size=m) # necessary to be kept separate from "other rsources" for this algorithmic formulation
job_limits[:9] = 2**32-1

min_cost = np.empty(n)
min_cost_producer = np.empty(n, dtype=int)
production_limit = np.empty(n, dtype=int)

for i in xrange(n):
    producers = np.where((A[i] < 0) * np.all(production_limit[:i,np.newaxis] >= A[:i], axis=0))[0]
    if producers.size == 0:
        min_cost[i] = 0.0 # -np.Inf seems better but makes problems later. Can't use this anyhow because production_limit is 0
        min_cost_producer[i] = -1
        production_limit[i] = 0
    else:
        produced_resource_costs = (c[producers] + min_cost[:i].dot(A[:i,producers])) / -A[i,producers]
        ix = np.argmax(produced_resource_costs)
        mcp = producers[ix]
        min_cost_producer[i] = mcp
        min_cost[i] = produced_resource_costs[ix]
        max_count = min([job_limits[mcp]] + [production_limit[j] / A[j,mcp] for j in xrange(i) if A[j,mcp] > 0])
        production_limit[i] = max_count * -A[i,mcp]

reduced_cost = c + min_cost.dot(A)
# ____ = c - max_value.dot(A) # to get a job value based on positive "max values", i.e. sell values
