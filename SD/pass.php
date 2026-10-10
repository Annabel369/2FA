<?php
$usuario = "creeper";
$senhaDigitada = "";
$hashGerado = "";
$linhaListPass = "";

if ($_SERVER["REQUEST_METHOD"] === "POST" && !empty($_POST["senha"])) {
    $usuario = !empty($_POST["usuario"]) ? trim($_POST["usuario"]) : "creeper";
    $senhaDigitada = $_POST["senha"];
    
    // Gera o Hash SHA-256 em letras minúsculas
    $hashGerado = hash("sha256", $senhaDigitada);
    
    // Formato pronto para o arquivo ListPass.txt do SD
    $linhaListPass = $usuario . "=" . $hashGerado;
}
?>
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Gerador SHA-256 - Creeper Auth</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #121212;
            color: #ffffff;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
        }
        .container {
            background-color: #1e1e1e;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 4px 15px rgba(0,0,0,0.6);
            width: 100%;
            max-width: 450px;
            border: 1px solid #333;
        }
        h2 {
            color: #55ff55;
            margin-top: 0;
            text-align: center;
        }
        label {
            display: block;
            margin-top: 15px;
            font-weight: bold;
            color: #bbb;
        }
        input[type="text"], input[type="password"] {
            width: 100%;
            padding: 10px;
            margin-top: 5px;
            border-radius: 5px;
            border: 1px solid #444;
            background-color: #2a2a2a;
            color: #fff;
            box-sizing: border-box;
            font-size: 15px;
        }
        button {
            margin-top: 20px;
            width: 100%;
            padding: 12px;
            background-color: #55ff55;
            color: #000;
            border: none;
            border-radius: 5px;
            font-weight: bold;
            font-size: 16px;
            cursor: pointer;
            transition: background 0.2s;
        }
        button:hover {
            background-color: #33cc33;
        }
        .result-box {
            margin-top: 20px;
            background-color: #000;
            padding: 12px;
            border-radius: 5px;
            border: 1px solid #55ff55;
            word-break: break-all;
            font-family: 'Courier New', Courier, monospace;
            color: #55ff55;
            font-size: 13px;
        }
    </style>
</head>
<body>

<div class="container">
    <h2>🟩 Gerador ListPass.txt</h2>

    <form method="POST" action="">
        <label for="usuario">Usuário:</label>
        <input type="text" id="usuario" name="usuario" value="<?php echo htmlspecialchars($usuario); ?>" required>

        <label for="senha">Nova Senha:</label>
        <input type="password" id="senha" name="senha" placeholder="Digite a senha desejada" required autofocus>

        <button type="submit">🔑 Gerar Hash SHA-256</button>
    </form>

    <?php if (!empty($hashGerado)): ?>
        <label>Linha para o ListPass.txt:</label>
        <div class="result-box" id="resultado"><?php echo htmlspecialchars($linhaListPass); ?></div>
        
        <label>Apenas o Hash (64 caracteres):</label>
        <div class="result-box"><?php echo htmlspecialchars($hashGerado); ?></div>
    <?php endif; ?>
</div>

</body>
</html>
