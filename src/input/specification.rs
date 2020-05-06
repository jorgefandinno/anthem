// TODO: refactor
fn term_assign_variable_declaration_domains<D>(term: &foliage::Term, declarations: &D)
	-> Result<(), crate::Error>
where
	D: crate::traits::AssignVariableDeclarationDomain,
{
	match term
	{
		foliage::Term::BinaryOperation(binary_operation) =>
		{
			term_assign_variable_declaration_domains(&binary_operation.left, declarations)?;
			term_assign_variable_declaration_domains(&binary_operation.right, declarations)?;
		},
		foliage::Term::Function(function) =>
			for argument in &function.arguments
			{
				term_assign_variable_declaration_domains(&argument, declarations)?;
			},
		foliage::Term::UnaryOperation(unary_operation) =>
			term_assign_variable_declaration_domains(&unary_operation.argument, declarations)?,
		foliage::Term::Variable(variable) =>
		{
			let domain =  match variable.declaration.name.chars().next()
			{
				Some('X')
				| Some('Y')
				| Some('Z') => crate::Domain::Program,
				Some('I')
				| Some('J')
				| Some('K')
				| Some('L')
				| Some('M')
				| Some('N') => crate::Domain::Integer,
				// TODO: improve error handling
				Some(other) => return Err(
					crate::Error::new_variable_name_not_allowed(variable.declaration.name.clone())),
				None => unreachable!(),
			};

			declarations.assign_variable_declaration_domain(&variable.declaration, domain);
		},
		_ => (),
	}

	Ok(())
}

fn formula_assign_variable_declaration_domains<D>(formula: &foliage::Formula, declarations: &D)
	-> Result<(), crate::Error>
where
	D: crate::traits::AssignVariableDeclarationDomain,
{
	match formula
	{
		foliage::Formula::And(arguments)
		| foliage::Formula::Or(arguments)
		| foliage::Formula::IfAndOnlyIf(arguments) =>
			for argument in arguments
			{
				formula_assign_variable_declaration_domains(&argument, declarations)?;
			},
		foliage::Formula::Compare(compare) =>
		{
			term_assign_variable_declaration_domains(&compare.left, declarations)?;
			term_assign_variable_declaration_domains(&compare.right, declarations)?;
		},
		foliage::Formula::Exists(quantified_formula)
		| foliage::Formula::ForAll(quantified_formula) =>
			formula_assign_variable_declaration_domains(&quantified_formula.argument,
				declarations)?,
		foliage::Formula::Implies(implies) =>
		{
			formula_assign_variable_declaration_domains(&implies.antecedent, declarations)?;
			formula_assign_variable_declaration_domains(&implies.implication, declarations)?;
		}
		foliage::Formula::Not(argument) =>
			formula_assign_variable_declaration_domains(&argument, declarations)?,
		foliage::Formula::Predicate(predicate) =>
			for argument in &predicate.arguments
			{
				term_assign_variable_declaration_domains(&argument, declarations)?;
			},
		_ => (),
	}

	Ok(())
}

fn closed_formula<'i, D>(input: &'i str, declarations: &D)
	-> Result<(crate::ScopedFormula, &'i str), crate::Error>
where
	D: foliage::FindOrCreateFunctionDeclaration
		+ foliage::FindOrCreatePredicateDeclaration
		+ crate::traits::AssignVariableDeclarationDomain,
{
	let terminator_position = match input.find('.')
	{
		None => return Err(crate::Error::new_missing_statement_terminator()),
		Some(terminator_position) => terminator_position,
	};

	let (formula_input, remaining_input) = input.split_at(terminator_position);
	let mut remaining_input_characters = remaining_input.chars();
	remaining_input_characters.next();
	let remaining_input = remaining_input_characters.as_str();

	let closed_formula = foliage::parse::formula(formula_input, declarations)
		.map_err(|error| crate::Error::new_parse_formula(error))?;

	formula_assign_variable_declaration_domains(&closed_formula.formula, declarations)?;

	// TODO: get rid of ScopedFormula
	let scoped_formula = crate::ScopedFormula
	{
		free_variable_declarations: closed_formula.free_variable_declarations,
		formula: closed_formula.formula,
	};

	Ok((scoped_formula, remaining_input))
}

fn variable_free_formula<'i, D>(input: &'i str, declarations: &D)
	-> Result<(foliage::Formula, &'i str), crate::Error>
where
	D: foliage::FindOrCreateFunctionDeclaration
		+ foliage::FindOrCreatePredicateDeclaration
		+ crate::traits::AssignVariableDeclarationDomain,
{
	let (closed_formula, input) = closed_formula(input, declarations)?;

	if !closed_formula.free_variable_declarations.is_empty()
	{
		// TODO: improve
		panic!("formula may not contain free variables");
	}

	Ok((closed_formula.formula, input))
}

fn formula_statement_body<'i>(input: &'i str, problem: &crate::Problem)
	-> Result<(foliage::Formula, &'i str), crate::Error>
{
	let input = input.trim_start();

	let mut input_characters = input.chars();

	let remaining_input = match input_characters.next()
	{
		Some(':') => input_characters.as_str(),
		_ => return Err(crate::Error::new_expected_colon()),
	};

	let input = remaining_input;

	variable_free_formula(input, problem)
}

fn input_statement_body<'i>(mut input: &'i str, problem: &crate::Problem)
	-> Result<&'i str, crate::Error>
{
	input = input.trim_start();

	let mut input_characters = input.chars();

	let remaining_input = match input_characters.next()
	{
		Some(':') => input_characters.as_str(),
		_ => return Err(crate::Error::new_expected_colon()),
	};

	input = remaining_input;

	loop
	{
		input = input.trim_start();

		let (constant_or_predicate_name, remaining_input) =
			foliage::parse::tokens::identifier(input)
				.ok_or_else(|| crate::Error::new_expected_identifier())?;

		input = remaining_input.trim_start();

		let mut input_characters = input.chars();

		match input_characters.next()
		{
			// Parse input predicate specifiers
			Some('/') =>
			{
				input = input_characters.as_str().trim_start();

				let (arity, remaining_input) = foliage::parse::tokens::number(input)
					.map_err(|error| crate::Error::new_parse_predicate_declaration().with(error))?
					.ok_or_else(|| crate::Error::new_parse_predicate_declaration())?;

				input = remaining_input.trim_start();

				let mut input_predicate_declarations =
					problem.input_predicate_declarations.borrow_mut();

				use foliage::FindOrCreatePredicateDeclaration;

				let predicate_declaration =
					problem.find_or_create_predicate_declaration(constant_or_predicate_name, arity);

				input_predicate_declarations.insert(predicate_declaration);

				let mut input_characters = input.chars();

				match input_characters.next()
				{
					Some(',') => input = input_characters.as_str(),
					_ => break,
				}
			},
			// Parse input constant specifiers
			Some(_)
			| None =>
			{
				let domain =
					if input.starts_with("->")
					{
						let mut input_characters = input.chars();
						input_characters.next();
						input_characters.next();

						input = input_characters.as_str().trim_start();

						let (identifier, remaining_input) =
							foliage::parse::tokens::identifier(input)
								.ok_or_else(|| crate::Error::new_expected_identifier())?;

						input = remaining_input;

						match identifier
						{
							"integer" => crate::Domain::Integer,
							"program" => crate::Domain::Program,
							_ => return Err(crate::Error::new_unknown_domain_identifier(
								identifier.to_string())),
						}
					}
					else
					{
						crate::Domain::Program
					};

				log::debug!("domain: {:?}", domain);

				let mut input_constant_declarations =
					problem.input_constant_declarations.borrow_mut();

				use foliage::FindOrCreateFunctionDeclaration;

				let constant_declaration =
					problem.find_or_create_function_declaration(constant_or_predicate_name, 0);

				input_constant_declarations.insert(std::rc::Rc::clone(&constant_declaration));

				let mut input_constant_declaration_domains =
					problem.input_constant_declaration_domains.borrow_mut();

				input_constant_declaration_domains.insert(constant_declaration, domain);

				let mut input_characters = input.chars();

				match input_characters.next()
				{
					Some(',') => input = input_characters.as_str(),
					_ => break,
				}
			}
		}
	}

	input = input.trim_start();

	let mut input_characters = input.chars();

	if input_characters.next() != Some('.')
	{
		return Err(crate::Error::new_missing_statement_terminator())
	}

	input = input_characters.as_str();

	Ok(input)
}

pub(crate) fn parse_specification(mut input: &str, problem: &crate::Problem)
	-> Result<(), crate::Error>
{
	loop
	{
		input = input.trim_start();

		if input.is_empty()
		{
			return Ok(());
		}

		let (identifier, remaining_input) = match foliage::parse::tokens::identifier(input)
		{
			Some(identifier) => identifier,
			None => return Err(crate::Error::new_expected_statement()),
		};

		input = remaining_input;

		match identifier
		{
			"axiom" =>
			{
				let (formula, remaining_input) = formula_statement_body(input, problem)?;
				input = remaining_input;

				let statement = crate::problem::Statement::new(
					crate::problem::StatementKind::Axiom, formula);

				problem.add_statement(crate::problem::SectionKind::Axioms, statement);

				continue;
			},
			"assume" =>
			{
				let (formula, remaining_input) = formula_statement_body(input, problem)?;
				input = remaining_input;

				let statement = crate::problem::Statement::new(
					crate::problem::StatementKind::Assumption, formula);

				problem.add_statement(crate::problem::SectionKind::Assumptions, statement);

				continue;
			},
			"lemma" =>
			{
				input = input.trim_start();

				let mut input_characters = input.chars();

				let (proof_direction, remaining_input) = match input_characters.next()
				{
					Some('(') =>
					{
						// TODO: refactor
						input = input_characters.as_str().trim_start();

						let (proof_direction, remaining_input) = match
							foliage::parse::tokens::identifier(input)
						{
							Some(("forward", remaining_input)) =>
								(crate::ProofDirection::Forward, remaining_input),
							Some(("backward", remaining_input)) =>
								(crate::ProofDirection::Backward, remaining_input),
							Some(("both", remaining_input)) =>
								(crate::ProofDirection::Both, remaining_input),
							Some((identifier, _)) =>
								return Err(crate::Error::new_unknown_proof_direction(
									identifier.to_string())),
							None => (crate::ProofDirection::Both, input),
						};

						input = remaining_input.trim_start();

						let mut input_characters = input.chars();

						if input_characters.next() != Some(')')
						{
							return Err(crate::Error::new_unmatched_parenthesis());
						}

						input = input_characters.as_str();

						(proof_direction, input)
					},
					Some(_)
					| None => (crate::ProofDirection::Both, remaining_input),
				};

				input = remaining_input;

				let (formula, remaining_input) = formula_statement_body(input, problem)?;

				input = remaining_input;

				let statement = crate::problem::Statement::new(
					crate::problem::StatementKind::Lemma(proof_direction), formula);

				problem.add_statement(crate::problem::SectionKind::Lemmas, statement);

				continue;
			},
			"assert" =>
			{
				let (formula, remaining_input) = formula_statement_body(input, problem)?;

				input = remaining_input;

				let statement = crate::problem::Statement::new(
					crate::problem::StatementKind::Assertion, formula);

				problem.add_statement(crate::problem::SectionKind::Assertions, statement);

				continue;
			},
			"input" => input = input_statement_body(input, problem)?,
			identifier => return Err(crate::Error::new_unknown_statement(identifier.to_string())),
		}
	}
}