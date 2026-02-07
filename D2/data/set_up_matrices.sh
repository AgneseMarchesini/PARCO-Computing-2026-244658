#!/bin/bash

echo "Matrix Download"

STRONG_DIR="D2/data/strong_scaling_matrices"
WEAK_DIR="D2/data/weak_scaling_matrices"
SCRIPT_DIR="D2/data"

mkdir -p "$STRONG_DIR"
mkdir -p "$WEAK_DIR"

declare -A STRONG_MATRICES=(
    ["venkat25.mtx"]="https://suitesparse-collection-website.herokuapp.com/MM/Simon/venkat25.tar.gz"
    ["atmosmodl.mtx"]="https://suitesparse-collection-website.herokuapp.com/MM/Bourchtein/atmosmodl.tar.gz"
    ["rajat31.mtx"]="https://suitesparse-collection-website.herokuapp.com/MM/Rajat/rajat31.tar.gz"
    ["circuit5M.mtx"]="https://suitesparse-collection-website.herokuapp.com/MM/Freescale/circuit5M.tar.gz"
    ["cage15.mtx"]="https://suitesparse-collection-website.herokuapp.com/MM/vanHeukelum/cage15.tar.gz"
)

download_matrix() {
    local MATRIX_FILE=$1
    local URL=$2
    local TARGET_DIR=$3

    FULL_PATH="$TARGET_DIR/$MATRIX_FILE"

    if [ -f "$FULL_PATH" ]; then
        SIZE=$(du -h "$FULL_PATH" | cut -f1)
        echo "$MATRIX_FILE already exists ($SIZE)"
        return 0
    fi

    echo "Downloading $MATRIX_FILE..."

    TAR_FILE="/tmp/${MATRIX_FILE}.tar.gz"
    EXTRACT_DIR="/tmp/${MATRIX_FILE}_extract"

    mkdir -p "$EXTRACT_DIR"

    wget --tries=3 --continue --progress=bar:force "$URL" -O "$TAR_FILE"
    if [ $? -ne 0 ]; then
        rm -f "$TAR_FILE"
        rmdir "$EXTRACT_DIR" 2>/dev/null
        echo "Failed to download $MATRIX_FILE from $URL"
        return 1
    fi

    echo "Extracting..."
    tar -xzf "$TAR_FILE" -C "$EXTRACT_DIR" 2>/dev/null

    MTX_FILE=$(find "$EXTRACT_DIR" -name "*.mtx" \
           ! -name "*_b.mtx" \
           ! -name "*_x.mtx" \
           -exec grep -l "matrix coordinate" {} \; | head -1)


    if [ -n "$MTX_FILE" ]; then
        mv "$MTX_FILE" "$FULL_PATH"
        SIZE=$(du -h "$FULL_PATH" | cut -f1)
        echo "Downloaded $MATRIX_FILE ($SIZE)"
        rm -f "$TAR_FILE"
        rm -rf "$EXTRACT_DIR"
        return 0
    fi

    # Cleanup on failure
    rm -f "$TAR_FILE"
    rm -rf "$EXTRACT_DIR"
    echo "Failed to download $MATRIX_FILE from $URL"
    return 1
}

# Download strong scaling matrices
echo ""
echo "Strong Scaling Matrices"
FAILED=0
for MATRIX_FILE in "${!STRONG_MATRICES[@]}"; do
    download_matrix "$MATRIX_FILE" "${STRONG_MATRICES[$MATRIX_FILE]}" "$STRONG_DIR" || FAILED=$((FAILED+1))
done

# Generating weak scaling matrices
echo ""
echo "Weak Scaling Matrices"

EXISTING_COUNT=$(ls -1 "$WEAK_DIR"/weak_*.mtx 2>/dev/null | wc -l)

if [ "$EXISTING_COUNT" -ge 8 ]; then
    echo "Weak scaling matrices already exist ($EXISTING_COUNT files)"
    ls -lh "$WEAK_DIR"/weak_*.mtx | awk '{printf "  %-30s %8s\n", $NF, $5}'
else
    echo "Generating weak scaling matrices with Python..."

    if [ ! -f "$SCRIPT_DIR/generate_weak_matrices.py" ]; then
        echo "Python script not found: $SCRIPT_DIR/generate_weak_matrices.py"
        FAILED=$((FAILED+1))
    else
        python3 "$SCRIPT_DIR/generate_weak_matrices.py"
        if [ $? -eq 0 ]; then
            echo "Weak scaling matrices generated successfully"
        else
            echo "Failed to generate weak scaling matrices"
            FAILED=$((FAILED+1))
        fi
    fi
fi

echo ""
echo "SUMMARY"
echo "Strong scaling:"
ls -lh "$STRONG_DIR"/*.mtx 2>/dev/null | awk '{printf "  %-35s %8s\n", $NF, $5}' || echo "  (none)"

echo ""
echo "Weak scaling:"
ls -lh "$WEAK_DIR"/*.mtx 2>/dev/null | awk '{printf "  %-35s %8s\n", $NF, $5}' || echo "  (none)"

echo ""
if [ "$FAILED" -eq 0 ]; then
    echo "Setup complete!"
    exit 0
else
    echo "Setup completed with $FAILED error(s)"
    exit 1
fi
