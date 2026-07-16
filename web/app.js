import createChessModule from "./engine/chess.js";

// =====================================================================
// Константы и утилиты
// =====================================================================

const STORAGE_KEY = "chess-web-state-v1";

const GLYPHS = {
	w: { p: "♙", n: "♘", b: "♗", r: "♖", q: "♕", k: "♔" },
	b: { p: "♟", n: "♞", b: "♝", r: "♜", q: "♛", k: "♚" },
};

const PROMOTION_ORDER = ["q", "r", "b", "n"];

function squareToXY(square) {
	return { x: square.charCodeAt(0) - 97, y: Number(square[1]) - 1 };
}

function xyToSquare(x, y) {
	return String.fromCharCode(97 + x) + (y + 1);
}

function fenToBoard(fen) {
	const placement = fen.split(" ")[0];
	const ranks = placement.split("/");
	const board = Array.from({ length: 8 }, () => Array(8).fill(null));
	for (let i = 0; i < ranks.length && i < 8; i++) {
		const y = 7 - i;
		let x = 0;
		for (const ch of ranks[i]) {
			if (ch >= "1" && ch <= "8") {
				x += Number(ch);
			} else {
				const color = ch === ch.toUpperCase() ? "w" : "b";
				const type = ch.toLowerCase();
				if (x >= 0 && x < 8 && y >= 0 && y < 8) {
					board[y][x] = { type, color };
				}
				x += 1;
			}
		}
	}
	return board;
}

function clamp(n, lo, hi) {
	return Math.max(lo, Math.min(hi, n));
}

// =====================================================================
// Состояние приложения
// =====================================================================

const state = {
	mode: "hh", // 'hh' | 'hb' | 'bb'
	players: { w: "human", b: "human" },
	humanColorPref: "w", // 'w' | 'b' | 'random'
	botDelayMs: 600,
	orientation: "w", // сторона, находящаяся внизу доски
	soundOn: true,
	autoplayPaused: false,

	history: [{ fen: "", lastMove: null }], // ply 0 - позиция без хода
	currentPly: 0,
	currentStatus: null, // результат Module.getStatus() для просматриваемой позиции
	gameOver: false,
	resignedBy: null, // 'w' | 'b' | null - относится к текущей "живой" партии

	selectedSquare: null,
	legalMovesForSelected: [],
	pendingPromotion: null,
	botTimer: null,
};

let Module = null;
const squaresMap = new Map();
const pieceElements = new Map();
let boardEl = null;

// =====================================================================
// Взаимодействие с WebAssembly-движком
// =====================================================================

function callStatus(fen) {
	return JSON.parse(Module.getStatus(fen));
}

function callMakeMove(fen, from, to, promotion) {
	return JSON.parse(Module.makeMove(fen, from, to, promotion || ""));
}

function callBotMove(fen) {
	return JSON.parse(Module.getBotMove(fen));
}

// =====================================================================
// Аудио (синтез звуков через Web Audio API - без внешних файлов)
// =====================================================================

let audioCtx = null;

function ensureAudio() {
	if (!audioCtx) {
		const Ctx = window.AudioContext || window.webkitAudioContext;
		if (!Ctx) return null;
		audioCtx = new Ctx();
	}
	if (audioCtx.state === "suspended") {
		audioCtx.resume().catch(() => {});
	}
	return audioCtx;
}

function primeAudioOnFirstGesture() {
	const handler = () => {
		ensureAudio();
		document.removeEventListener("pointerdown", handler);
		document.removeEventListener("keydown", handler);
	};
	document.addEventListener("pointerdown", handler, { once: true });
	document.addEventListener("keydown", handler, { once: true });
}

function tone(ctx, { freq, start, duration, type = "sine", peak = 0.22, endFreq = null }) {
	const osc = ctx.createOscillator();
	const gain = ctx.createGain();
	osc.type = type;
	osc.frequency.setValueAtTime(freq, start);
	if (endFreq) {
		osc.frequency.exponentialRampToValueAtTime(endFreq, start + duration);
	}
	gain.gain.setValueAtTime(0.0001, start);
	gain.gain.exponentialRampToValueAtTime(peak, start + 0.012);
	gain.gain.exponentialRampToValueAtTime(0.0001, start + duration);
	osc.connect(gain).connect(ctx.destination);
	osc.start(start);
	osc.stop(start + duration + 0.03);
}

const SOUND_FNS = {
	move(ctx) {
		const t = ctx.currentTime;
		tone(ctx, { freq: 520, start: t, duration: 0.09, type: "triangle", peak: 0.22 });
	},
	capture(ctx) {
		const t = ctx.currentTime;
		tone(ctx, { freq: 260, start: t, duration: 0.16, type: "sawtooth", peak: 0.22, endFreq: 150 });
		tone(ctx, { freq: 180, start: t + 0.02, duration: 0.12, type: "square", peak: 0.12 });
	},
	check(ctx) {
		const t = ctx.currentTime;
		tone(ctx, { freq: 523, start: t, duration: 0.11, type: "square", peak: 0.18 });
		tone(ctx, { freq: 659, start: t + 0.12, duration: 0.13, type: "square", peak: 0.18 });
	},
	promote(ctx) {
		const t = ctx.currentTime;
		[440, 554, 659, 880].forEach((f, i) => {
			tone(ctx, { freq: f, start: t + i * 0.08, duration: 0.13, type: "triangle", peak: 0.18 });
		});
	},
	mate(ctx) {
		const t = ctx.currentTime;
		[392, 330, 262].forEach((f, i) => {
			tone(ctx, { freq: f, start: t + i * 0.17, duration: 0.32, type: "sawtooth", peak: 0.2 });
		});
	},
	draw(ctx) {
		const t = ctx.currentTime;
		tone(ctx, { freq: 330, start: t, duration: 0.26, type: "sine", peak: 0.18 });
		tone(ctx, { freq: 330, start: t + 0.3, duration: 0.26, type: "sine", peak: 0.18 });
	},
};

function playSound(name) {
	if (!state.soundOn) return;
	const ctx = ensureAudio();
	if (!ctx) return;
	try {
		SOUND_FNS[name] && SOUND_FNS[name](ctx);
	} catch (e) {
		/* игнорируем ошибки аудио (например, ограничения автовоспроизведения) */
	}
}

function playSoundsForMove(moveResult, status) {
	if (status.outcome === "mate") {
		playSound("mate");
		return;
	}
	if (status.outcome === "stalemate") {
		playSound("draw");
		return;
	}
	if (moveResult.promotion) {
		playSound("promote");
		return;
	}
	if (status.inCheck) {
		playSound("check");
		return;
	}
	if (moveResult.isCapture) {
		playSound("capture");
		return;
	}
	playSound("move");
}

// =====================================================================
// Персистентность (localStorage)
// =====================================================================

function persistState() {
	const liveFen = state.history[state.history.length - 1].fen;
	const data = {
		version: 1,
		fen: liveFen,
		mode: state.mode,
		players: state.players,
		humanColorPref: state.humanColorPref,
		botDelayMs: state.botDelayMs,
		orientation: state.orientation,
		soundOn: state.soundOn,
		autoplayPaused: state.autoplayPaused,
		gameOver: state.gameOver,
		resignedBy: state.resignedBy,
	};
	try {
		localStorage.setItem(STORAGE_KEY, JSON.stringify(data));
	} catch (e) {
		/* localStorage может быть недоступен (приватный режим и т.п.) */
	}
}

function readPersisted() {
	try {
		const raw = localStorage.getItem(STORAGE_KEY);
		if (!raw) return null;
		const data = JSON.parse(raw);
		if (!data || typeof data.fen !== "string") return null;
		return data;
	} catch (e) {
		return null;
	}
}

// =====================================================================
// Рендеринг доски
// =====================================================================

function visualToXY(vr, vc) {
	if (state.orientation === "w") {
		return { x: vc, y: 7 - vr };
	}
	return { x: 7 - vc, y: vr };
}

function squareToPercentPos(square) {
	const { x, y } = squareToXY(square);
	let vr, vc;
	if (state.orientation === "w") {
		vr = 7 - y;
		vc = x;
	} else {
		vr = y;
		vc = 7 - x;
	}
	return { leftPct: vc * 12.5, topPct: vr * 12.5 };
}

function buildSquaresGrid() {
	boardEl.innerHTML = "";
	squaresMap.clear();

	for (let vr = 0; vr < 8; vr++) {
		for (let vc = 0; vc < 8; vc++) {
			const { x, y } = visualToXY(vr, vc);
			const square = xyToSquare(x, y);
			const div = document.createElement("div");
			div.className = "square " + ((x + y) % 2 === 0 ? "square--dark" : "square--light");
			div.dataset.square = square;

			if (vr === 7) {
				const fileLabel = document.createElement("span");
				fileLabel.className = "coord coord--file";
				fileLabel.textContent = String.fromCharCode(97 + x);
				div.appendChild(fileLabel);
			}
			if (vc === 0) {
				const rankLabel = document.createElement("span");
				rankLabel.className = "coord coord--rank";
				rankLabel.textContent = String(y + 1);
				div.appendChild(rankLabel);
			}

			div.addEventListener("click", () => onSquareClick(square));
			boardEl.appendChild(div);
			squaresMap.set(square, div);
		}
	}

	// Фигуры - отдельный слой поверх сетки клеток.
	for (const el of pieceElements.values()) {
		boardEl.appendChild(el);
	}
}

function positionPieceElement(el, square) {
	const { leftPct, topPct } = squareToPercentPos(square);
	el.style.left = leftPct + "%";
	el.style.top = topPct + "%";
}

function createPieceElement(type, color, square) {
	const el = document.createElement("div");
	el.className = "piece piece--" + color;
	el.textContent = GLYPHS[color][type];
	el.dataset.square = square;
	positionPieceElement(el, square);
	return el;
}

function snapRenderBoard(fen) {
	for (const el of pieceElements.values()) {
		el.remove();
	}
	pieceElements.clear();

	const boardArr = fenToBoard(fen);
	for (let y = 0; y < 8; y++) {
		for (let x = 0; x < 8; x++) {
			const cell = boardArr[y][x];
			if (cell) {
				const sq = xyToSquare(x, y);
				const el = createPieceElement(cell.type, cell.color, sq);
				boardEl.appendChild(el);
				pieceElements.set(sq, el);
			}
		}
	}
}

function movePieceElement(fromSq, toSq) {
	const el = pieceElements.get(fromSq);
	if (!el) return;
	pieceElements.delete(fromSq);
	pieceElements.set(toSq, el);
	el.dataset.square = toSq;
	positionPieceElement(el, toSq);
}

function removePieceElementAnimated(sq) {
	const el = pieceElements.get(sq);
	if (!el) return;
	pieceElements.delete(sq);
	el.classList.add("piece--captured");
	setTimeout(() => el.remove(), 220);
}

function setPieceType(sq, type, color) {
	const el = pieceElements.get(sq);
	if (!el) return;
	el.textContent = GLYPHS[color][type];
}

function animateApplyMove(moveResult) {
	const { from, to, isCastle, isEnPassant, capturedPiece, promotion, color } = moveResult;

	let capturedSquare = to;
	if (isEnPassant) {
		capturedSquare = to[0] + from[1];
	}
	if (capturedPiece) {
		removePieceElementAnimated(capturedSquare);
	}

	movePieceElement(from, to);

	if (promotion) {
		setPieceType(to, promotion, color);
	}

	if (isCastle) {
		const rank = from[1];
		if (to[0] === "g") {
			movePieceElement("h" + rank, "f" + rank);
		} else if (to[0] === "c") {
			movePieceElement("a" + rank, "d" + rank);
		}
	}
}

function findKingSquare(boardArr, color) {
	for (let y = 0; y < 8; y++) {
		for (let x = 0; x < 8; x++) {
			const c = boardArr[y][x];
			if (c && c.type === "k" && c.color === color) {
				return xyToSquare(x, y);
			}
		}
	}
	return null;
}

function updateHighlights() {
	for (const el of squaresMap.values()) {
		el.classList.remove(
			"sq--last-from",
			"sq--last-to",
			"sq--selected",
			"sq--legal-move",
			"sq--legal-capture",
			"sq--check",
			"sq--mate"
		);
	}

	const ply = state.history[state.currentPly];
	if (ply.lastMove) {
		squaresMap.get(ply.lastMove.from)?.classList.add("sq--last-from");
		squaresMap.get(ply.lastMove.to)?.classList.add("sq--last-to");
	}

	if (state.selectedSquare) {
		squaresMap.get(state.selectedSquare)?.classList.add("sq--selected");

		const boardArr = fenToBoard(ply.fen);
		const { x: sx, y: sy } = squareToXY(state.selectedSquare);
		const selectedPiece = boardArr[sy][sx];

		for (const mv of state.legalMovesForSelected) {
			const { x, y } = squareToXY(mv.to);
			const occupied = boardArr[y][x] !== null;
			const isPawnDiagonalToEmpty = selectedPiece && selectedPiece.type === "p" && x !== sx && !occupied;
			const isCapture = occupied || isPawnDiagonalToEmpty;
			squaresMap.get(mv.to)?.classList.add(isCapture ? "sq--legal-capture" : "sq--legal-move");
		}
	}

	if (state.currentStatus && state.currentStatus.inCheck) {
		const boardArr = fenToBoard(ply.fen);
		const kingSq = findKingSquare(boardArr, state.currentStatus.colorToMove);
		if (kingSq) {
			const cls = state.currentStatus.outcome === "mate" ? "sq--mate" : "sq--check";
			squaresMap.get(kingSq)?.classList.add(cls);
		}
	}
}

function render({ animateMoveResult = null } = {}) {
	const fen = state.history[state.currentPly].fen;
	if (animateMoveResult) {
		animateApplyMove(animateMoveResult);
	} else {
		snapRenderBoard(fen);
	}
	updateHighlights();
	updateStatusBar();
	updateMoveList();
	updateControlsEnabled();
	updateFenDisplay(fen);
}

// =====================================================================
// Статус, список ходов, элементы управления
// =====================================================================

function updateStatusBar() {
	const el = document.getElementById("status-bar");
	const viewingLive = state.currentPly === state.history.length - 1;
	let msg = "";

	if (viewingLive && state.resignedBy) {
		msg = state.resignedBy === "w" ? "Белые сдались. Победили чёрные." : "Чёрные сдались. Победили белые.";
	} else if (state.currentStatus) {
		const { colorToMove, inCheck, outcome } = state.currentStatus;
		if (outcome === "mate") {
			const winner = colorToMove === "w" ? "чёрные" : "белые";
			msg = `Мат! Победили ${winner}.`;
		} else if (outcome === "stalemate") {
			msg = "Ничья (пат).";
		} else {
			msg = (colorToMove === "w" ? "Ходят белые" : "Ходят чёрные") + (inCheck ? " — Шах!" : "") + ".";
		}
	}

	if (!viewingLive) {
		msg += " (просмотр истории)";
	}

	el.textContent = msg;
}

function promotionSuffix(mv) {
	if (!mv.promotion) return "";
	return "=" + GLYPHS[mv.color][mv.promotion];
}

function updateMoveList() {
	const container = document.getElementById("move-list");
	container.innerHTML = "";

	for (let i = 1; i < state.history.length; i++) {
		const mv = state.history[i].lastMove;
		const li = document.createElement("li");
		li.className = "move-list__item" + (i === state.currentPly ? " move-list__item--active" : "");
		const moveNumber = Math.ceil(i / 2);
		const isWhiteMove = mv.color === "w";
		const label = `${mv.from}\u2013${mv.to}${promotionSuffix(mv)}`;
		li.textContent = (isWhiteMove ? `${moveNumber}. ` : "") + label;
		li.addEventListener("click", () => goToPly(i));
		container.appendChild(li);
	}

	container.scrollTop = container.scrollHeight;
}

function updateControlsEnabled() {
	document.getElementById("btn-first").disabled = state.currentPly === 0;
	document.getElementById("btn-prev").disabled = state.currentPly === 0;
	document.getElementById("btn-next").disabled = state.currentPly === state.history.length - 1;
	document.getElementById("btn-last").disabled = state.currentPly === state.history.length - 1;
	document.getElementById("btn-undo").disabled = !state.resignedBy && state.history.length <= 1;
}

function updateFenDisplay(fen) {
	document.getElementById("fen-display").value = fen;
}

function updateSoundIcon() {
	document.getElementById("btn-sound").textContent = state.soundOn ? "🔊" : "🔇";
}

function syncSettingsUI() {
	document.getElementById("select-mode").value = state.mode;
	document.getElementById("select-human-color").value = state.humanColorPref;
	document.getElementById("select-bot-delay").value = String(state.botDelayMs);

	const hasBot = state.mode !== "hh";
	document.getElementById("bot-delay-row").hidden = !hasBot;
	document.getElementById("human-color-row").hidden = state.mode !== "hb";

	const pauseBtn = document.getElementById("btn-pause");
	pauseBtn.hidden = !hasBot;
	pauseBtn.textContent = state.autoplayPaused ? "Продолжить" : "Пауза";

	document.getElementById("btn-resign-w").hidden = state.players.w !== "human";
	document.getElementById("btn-resign-b").hidden = state.players.b !== "human";

	updateSoundIcon();
}

// =====================================================================
// Тосты
// =====================================================================

let toastTimer = null;

function showToast(message) {
	const el = document.getElementById("toast");
	el.textContent = message;
	el.hidden = false;
	// принудительный reflow, чтобы гарантированно сработала transition при повторном вызове
	void el.offsetWidth;
	el.classList.add("toast--visible");
	clearTimeout(toastTimer);
	toastTimer = setTimeout(() => {
		el.classList.remove("toast--visible");
		setTimeout(() => {
			el.hidden = true;
		}, 260);
	}, 2200);
}

// =====================================================================
// Логика хода: выбор клетки, превращение
// =====================================================================

function isHumanTurnToMove() {
	if (!state.currentStatus) return false;
	if (state.currentStatus.outcome !== "ongoing") return false;
	if (state.resignedBy && state.currentPly === state.history.length - 1) return false;
	return state.players[state.currentStatus.colorToMove] === "human";
}

function isSelectable(square) {
	if (!isHumanTurnToMove()) return false;
	return state.currentStatus.legalMoves.some((m) => m.from === square);
}

function selectSquare(square) {
	state.selectedSquare = square;
	state.legalMovesForSelected = state.currentStatus.legalMoves.filter((m) => m.from === square);
	updateHighlights();
}

function clearSelection() {
	state.selectedSquare = null;
	state.legalMovesForSelected = [];
	updateHighlights();
}

function onSquareClick(square) {
	if (state.pendingPromotion) return;
	if (!isHumanTurnToMove()) return;

	if (state.selectedSquare) {
		const group = state.legalMovesForSelected.find((m) => m.to === square);
		if (group) {
			const from = state.selectedSquare;
			if (group.promotions.length) {
				openPromotionDialog(from, square, group.promotions);
			} else {
				clearSelection();
				applyMove(from, square, "");
			}
			return;
		}

		if (square === state.selectedSquare) {
			clearSelection();
			return;
		}

		if (isSelectable(square)) {
			selectSquare(square);
		} else {
			clearSelection();
		}
		return;
	}

	if (isSelectable(square)) {
		selectSquare(square);
	}
}

function openPromotionDialog(from, to, promotions) {
	state.pendingPromotion = { from, to };
	const modal = document.getElementById("promotion-modal");
	const optionsEl = document.getElementById("promotion-options");
	optionsEl.innerHTML = "";

	const color = state.currentStatus.colorToMove;
	for (const p of PROMOTION_ORDER) {
		if (!promotions.includes(p)) continue;
		const btn = document.createElement("button");
		btn.type = "button";
		btn.className = "promotion-btn";
		btn.textContent = GLYPHS[color][p];
		btn.addEventListener("click", () => {
			const pending = state.pendingPromotion;
			closePromotionDialog();
			clearSelection();
			applyMove(pending.from, pending.to, p);
		});
		optionsEl.appendChild(btn);
	}

	modal.hidden = false;
}

function closePromotionDialog() {
	state.pendingPromotion = null;
	document.getElementById("promotion-modal").hidden = true;
}

// =====================================================================
// Выполнение хода, бот, навигация по истории
// =====================================================================

function clearBotTimer() {
	if (state.botTimer) {
		clearTimeout(state.botTimer);
		state.botTimer = null;
	}
}

function scheduleBotMoveIfNeeded() {
	clearBotTimer();
	if (state.autoplayPaused) return;
	if (state.currentPly !== state.history.length - 1) return;
	if (!state.currentStatus) return;
	if (state.currentStatus.outcome !== "ongoing") return;
	if (state.resignedBy) return;
	if (state.players[state.currentStatus.colorToMove] !== "bot") return;

	state.botTimer = setTimeout(() => {
		state.botTimer = null;
		performBotMove();
	}, state.botDelayMs);
}

function performBotMove() {
	const fen = state.history[state.currentPly].fen;
	const resp = callBotMove(fen);
	if (!resp.ok) return;
	applyMove(resp.from, resp.to, resp.promotion);
}

function applyMove(from, to, promotion) {
	const fen = state.history[state.currentPly].fen;
	const result = callMakeMove(fen, from, to, promotion);
	if (!result.ok) {
		showToast(result.error || "Недопустимый ход");
		return;
	}

	state.history = state.history.slice(0, state.currentPly + 1);
	state.history.push({ fen: result.fen, lastMove: result });
	state.currentPly = state.history.length - 1;
	state.resignedBy = null;

	state.currentStatus = callStatus(result.fen);
	state.gameOver = state.currentStatus.outcome !== "ongoing";

	clearSelection();
	render({ animateMoveResult: result });
	playSoundsForMove(result, state.currentStatus);
	persistState();
	scheduleBotMoveIfNeeded();
}

function goToPly(n) {
	clearBotTimer();
	state.currentPly = clamp(n, 0, state.history.length - 1);
	const fen = state.history[state.currentPly].fen;
	state.currentStatus = callStatus(fen);
	clearSelection();
	render({});
	scheduleBotMoveIfNeeded();
}

function undo() {
	clearBotTimer();

	if (state.resignedBy) {
		state.resignedBy = null;
		state.gameOver = false;
	} else if (state.history.length > 1) {
		state.history.pop();

		// Если следующим ходом должен ходить бот (а в партии участвует хотя бы
		// один человек), откатываемся ещё дальше - до ближайшего решения человека.
		// Это стандартное поведение "отмены хода" против бота: отменяется и ответ
		// бота, и предшествовавший ему ход игрока.
		const hasHuman = state.players.w === "human" || state.players.b === "human";
		if (hasHuman) {
			while (state.history.length > 1) {
				const probe = callStatus(state.history[state.history.length - 1].fen);
				if (probe.outcome !== "ongoing") break;
				if (state.players[probe.colorToMove] === "human") break;
				state.history.pop();
			}
		}
	} else {
		return;
	}

	state.currentPly = state.history.length - 1;
	const fen = state.history[state.currentPly].fen;
	state.currentStatus = callStatus(fen);
	state.gameOver = state.currentStatus.outcome !== "ongoing";

	clearSelection();
	render({});
	persistState();
	scheduleBotMoveIfNeeded();
}

function resign(color) {
	if (state.gameOver || state.resignedBy) return;
	clearBotTimer();
	state.resignedBy = color;
	state.gameOver = true;
	clearSelection();
	render({});
	playSound("mate");
	persistState();
}

function computePlayersForNewGame() {
	if (state.mode === "hh") {
		return { w: "human", b: "human" };
	}
	if (state.mode === "bb") {
		return { w: "bot", b: "bot" };
	}
	// 'hb'
	let color = state.humanColorPref;
	if (color === "random") {
		color = Math.random() < 0.5 ? "w" : "b";
	}
	return color === "w" ? { w: "human", b: "bot" } : { w: "bot", b: "human" };
}

function newGame() {
	clearBotTimer();
	state.players = computePlayersForNewGame();

	const startFen = Module.getStartFEN();
	state.history = [{ fen: startFen, lastMove: null }];
	state.currentPly = 0;
	state.gameOver = false;
	state.resignedBy = null;
	state.autoplayPaused = false;

	state.currentStatus = callStatus(startFen);

	clearSelection();
	syncSettingsUI();
	render({});
	persistState();
	scheduleBotMoveIfNeeded();
}

function loadFEN(fenRaw) {
	const trimmed = (fenRaw || "").trim();
	if (!trimmed) {
		showToast("Введите позицию в формате FEN");
		return;
	}

	const status = callStatus(trimmed);
	if (!status.ok) {
		showToast(status.error || "Некорректный FEN");
		return;
	}

	clearBotTimer();
	state.history = [{ fen: trimmed, lastMove: null }];
	state.currentPly = 0;
	state.gameOver = status.outcome !== "ongoing";
	state.resignedBy = null;
	state.currentStatus = status;

	clearSelection();
	render({});
	persistState();
	scheduleBotMoveIfNeeded();
}

function toggleOrientation() {
	state.orientation = state.orientation === "w" ? "b" : "w";
	buildSquaresGrid();
	render({});
	persistState();
}

function togglePause() {
	state.autoplayPaused = !state.autoplayPaused;
	syncSettingsUI();
	persistState();
	scheduleBotMoveIfNeeded();
}

function toggleSound() {
	state.soundOn = !state.soundOn;
	updateSoundIcon();
	persistState();
}

// =====================================================================
// Инициализация и связывание UI
// =====================================================================

function wireUI() {
	document.getElementById("btn-flip").addEventListener("click", toggleOrientation);
	document.getElementById("btn-sound").addEventListener("click", toggleSound);

	document.getElementById("btn-new-game").addEventListener("click", newGame);
	document.getElementById("btn-pause").addEventListener("click", togglePause);
	document.getElementById("btn-undo").addEventListener("click", undo);

	document.getElementById("btn-first").addEventListener("click", () => goToPly(0));
	document.getElementById("btn-prev").addEventListener("click", () => goToPly(state.currentPly - 1));
	document.getElementById("btn-next").addEventListener("click", () => goToPly(state.currentPly + 1));
	document.getElementById("btn-last").addEventListener("click", () => goToPly(state.history.length - 1));

	document.getElementById("btn-resign-w").addEventListener("click", () => resign("w"));
	document.getElementById("btn-resign-b").addEventListener("click", () => resign("b"));

	document.getElementById("select-mode").addEventListener("change", (e) => {
		state.mode = e.target.value;
		syncSettingsUI();
		newGame();
	});
	document.getElementById("select-human-color").addEventListener("change", (e) => {
		state.humanColorPref = e.target.value;
		newGame();
	});
	document.getElementById("select-bot-delay").addEventListener("change", (e) => {
		state.botDelayMs = Number(e.target.value);
		persistState();
		scheduleBotMoveIfNeeded();
	});

	document.getElementById("btn-copy-fen").addEventListener("click", async () => {
		const text = document.getElementById("fen-display").value;
		try {
			await navigator.clipboard.writeText(text);
			showToast("FEN скопирован в буфер обмена");
		} catch (e) {
			const ta = document.getElementById("fen-display");
			ta.select();
			try {
				document.execCommand("copy");
				showToast("FEN скопирован в буфер обмена");
			} catch (e2) {
				showToast("Не удалось скопировать FEN");
			}
		}
	});

	document.getElementById("btn-load-fen").addEventListener("click", () => {
		loadFEN(document.getElementById("fen-input").value);
	});

	document.getElementById("promotion-modal").addEventListener("click", (e) => {
		if (e.target.id === "promotion-modal") {
			// клик по подложке не должен молча отменять выбор превращения без явного действия -
			// оставляем модальное окно открытым, чтобы игрок обязательно выбрал фигуру.
		}
	});
}

function loadPersistedOrDefault() {
	const saved = readPersisted();
	if (saved) {
		state.mode = saved.mode || "hh";
		state.players = saved.players || { w: "human", b: "human" };
		state.humanColorPref = saved.humanColorPref || "w";
		state.botDelayMs = typeof saved.botDelayMs === "number" ? saved.botDelayMs : 600;
		state.orientation = saved.orientation === "b" ? "b" : "w";
		state.soundOn = saved.soundOn !== false;
		state.autoplayPaused = !!saved.autoplayPaused;
		state.gameOver = !!saved.gameOver;
		state.resignedBy = saved.resignedBy || null;

		const status = callStatus(saved.fen);
		if (status.ok) {
			state.history = [{ fen: saved.fen, lastMove: null }];
		} else {
			state.history = [{ fen: Module.getStartFEN(), lastMove: null }];
			state.gameOver = false;
			state.resignedBy = null;
		}
	} else {
		state.history = [{ fen: Module.getStartFEN(), lastMove: null }];
	}
	state.currentPly = 0;
}

async function init() {
	boardEl = document.getElementById("board");

	Module = await createChessModule();

	loadPersistedOrDefault();
	buildSquaresGrid();
	wireUI();
	primeAudioOnFirstGesture();

	const fen = state.history[state.currentPly].fen;
	state.currentStatus = callStatus(fen);

	syncSettingsUI();
	render({});
	scheduleBotMoveIfNeeded();

	document.getElementById("loading-overlay").hidden = true;
}

init().catch((err) => {
	console.error(err);
	const overlay = document.getElementById("loading-overlay");
	overlay.querySelector(".loading-overlay__text").textContent =
		"Не удалось загрузить шахматный движок. Обновите страницу.";
});
