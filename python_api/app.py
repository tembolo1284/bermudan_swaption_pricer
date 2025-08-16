from fastapi import FastAPI, Query
from pydantic import BaseModel, Field
import bermudan_native  # built by CMake; ensure PYTHONPATH includes build dir

app = FastAPI(title="Bermudan Swaption Pricer API", version="1.0.0")

class PriceRequest(BaseModel):
    date: str = Field(..., description="YYYY-MM-DD, e.g. 2025-07-15")
    flat_rate: float = Field(..., ge=0, le=1, description="Flat curve rate, e.g. 0.035")
    model: str = Field(..., pattern="^(g2|hw|bk)$")
    engine: str = Field(..., pattern="^(tree|fdm)$")
    strike_multiplier: float = Field(..., gt=0, description="1.0=ATM, 1.2=OTM, 0.8=ITM")

def parse_date(s: str):
    y, m, d = map(int, s.split("-"))
    return y, m, d

@app.get("/healthz")
def health():
    return {"status": "ok"}

@app.post("/price")
def price(req: PriceRequest):
    y, m, d = parse_date(req.date)
    npv = bermudan_native.price_bermudan(
        y, m, d, req.flat_rate, req.model, req.engine, req.strike_multiplier
    )
    return {
        "npv": npv,
        "inputs": req.model_dump()
    }

